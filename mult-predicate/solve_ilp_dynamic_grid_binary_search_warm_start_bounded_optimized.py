import gurobipy as gp
from gurobipy import GRB
import time
import sys
import traceback
import bisect

DEBUG = False

def build_model(points, input_box_indices, qbox_bounds, epsilon, d=2, delta_base=2, use_dynamic_grid=True, warm_start_box=None, K_add=None, K_drop=None, color_weights=None, coverage=False, min_coverage=None, fairness_rules=None, diff_matrix=None, ratio_matrix=None):
    n = len(points)
    
    # Pre-mark points in input box
    for i, p in enumerate(points):
        p['in_I'] = (i in input_box_indices)

    # --- Coordinate compression ---
    sorted_coords = []
    for k in range(d):
        vals = sorted(set(p['coords'][k] for p in points))
        sorted_coords.append(vals)

    coord_maps = [{v: i for i, v in enumerate(vals)} for vals in sorted_coords]

    point_ranks = [[0]*d for _ in range(n)]
    for i in range(n):
        for k in range(d):
            point_ranks[i][k] = coord_maps[k][points[i]['coords'][k]]

    # --- Dynamic Grid Generation ---
    allowed_L_vars = []
    allowed_R_vars = []
    L_ranks = []
    R_ranks = []
    for k in range(d):
        m = len(sorted_coords[k])

        L_in = qbox_bounds[k][0]
        R_in = qbox_bounds[k][1]

        L_rank = bisect.bisect_left(sorted_coords[k], L_in)
        if L_rank >= m: L_rank = m - 1
        R_rank = bisect.bisect_right(sorted_coords[k], R_in) - 1
        if R_rank < 0: R_rank = 0

        L_ranks.append(int(L_rank))
        R_ranks.append(int(R_rank))

        if not use_dynamic_grid:
            if K_add is not None and K_drop is not None:
                min_L = max(0, int(L_rank) - K_add)
                max_L = min(int(R_rank), int(L_rank) + K_drop)
                allowed_L_vars.append(list(range(min_L, max_L + 1)))

                min_R = max(int(L_rank), int(R_rank) - K_drop)
                max_R = min(m - 1, int(R_rank) + K_add)
                allowed_R_vars.append(list(range(min_R, max_R + 1)))
            else:
                allowed_L_vars.append(list(range(0, int(R_rank) + 1)))
                allowed_R_vars.append(list(range(int(L_rank), m)))
            continue

        # L valid ranks (dynamic grid)
        allowed_L = set([0, int(L_rank)])
        step = 1
        while L_rank - step > 0:
            allowed_L.add(int(L_rank - step))
            step = max(step + 1, int(step * delta_base))
        step = 1
        while L_rank + step < R_rank:
            allowed_L.add(int(L_rank + step))
            step = max(step + 1, int(step * delta_base))

        # R valid ranks (dynamic grid)
        allowed_R = set([m - 1, int(R_rank)])
        step = 1
        while R_rank + step < m - 1:
            allowed_R.add(int(R_rank + step))
            step = max(step + 1, int(step * delta_base))
        step = 1
        while R_rank - step > L_rank:
            allowed_R.add(int(R_rank - step))
            step = max(step + 1, int(step * delta_base))

        allowed_L_vars.append(sorted(list(allowed_L)))
        allowed_R_vars.append(sorted(list(allowed_R)))

    # Calculate allowed boundary ranges
    min_L_rank = [min(allowed_L_vars[k]) for k in range(d)]
    max_L_rank = [max(allowed_L_vars[k]) for k in range(d)]
    min_R_rank = [min(allowed_R_vars[k]) for k in range(d)]
    max_R_rank = [max(allowed_R_vars[k]) for k in range(d)]

    # Filter out guaranteed-out points completely to reduce Gurobi variable count
    filtered_points = []
    filtered_point_ranks = []
    for i in range(n):
        is_out = False
        for k in range(d):
            r_ik = point_ranks[i][k]
            if r_ik < min_L_rank[k] or r_ik > max_R_rank[k]:
                is_out = True
                break
        if not is_out:
            filtered_points.append(points[i])
            filtered_point_ranks.append(point_ranks[i])

    n_filtered = len(filtered_points)

    # Classification of filtered points:
    # 1: guaranteed to be in the chosen box
    # -1: variable (depends on solver decision)
    point_status = []
    for i in range(n_filtered):
        is_in = True
        ranks = filtered_point_ranks[i]
        for k in range(d):
            if not (ranks[k] >= max_L_rank[k] and ranks[k] <= min_R_rank[k]):
                is_in = False
                break
        if is_in:
            point_status.append(1)
        else:
            point_status.append(-1)

    # --- Model ---
    model = gp.Model("FairRangeFeasibility")
    model.setParam("OutputFlag", 0)

    # --- Boundary variables ---
    l_vars, r_vars, pos_L, pos_R = [], [], [], []
    for k in range(d):
        l = {}
        for j in allowed_L_vars[k]:
            l[j] = model.addVar(vtype=GRB.BINARY, name=f"L_{k}_{j}")
        r = {}
        for j in allowed_R_vars[k]:
            r[j] = model.addVar(vtype=GRB.BINARY, name=f"R_{k}_{j}")

        if warm_start_box is not None:
            val_L = warm_start_box[k][0]
            if val_L in sorted_coords[k]:
                idx_L = sorted_coords[k].index(val_L)
                if idx_L in l:
                    l[idx_L].Start = 1

            val_R = warm_start_box[k][1]
            if val_R in sorted_coords[k]:
                idx_R = sorted_coords[k].index(val_R)
                if idx_R in r:
                    r[idx_R].Start = 1

        l_vars.append(l)
        r_vars.append(r)

        model.addConstr(gp.quicksum(l.values()) == 1)
        model.addConstr(gp.quicksum(r.values()) == 1)

        posL_expr = gp.quicksum(j * l[j] for j in allowed_L_vars[k])
        posR_expr = gp.quicksum(j * r[j] for j in allowed_R_vars[k])

        posL_var = model.addVar(vtype=GRB.INTEGER, name=f"posL_val_{k}", ub=R_ranks[k])
        posR_var = model.addVar(vtype=GRB.INTEGER, name=f"posR_val_{k}", lb=L_ranks[k])

        model.addConstr(posL_var == posL_expr)
        model.addConstr(posR_var == posR_expr)
        model.addConstr(posL_var <= posR_var)

        pos_L.append(posL_var)
        pos_R.append(posR_var)

    # --- Output indicator ---
    O = {}
    for i in range(n_filtered):
        status = point_status[i]
        if status == 1:
            O[i] = 1.0
        else:
            O[i] = model.addVar(vtype=GRB.BINARY, name=f"O_{i}")

    # --- Dimension satisfaction indicators (only for variable points) ---
    Z = {}
    M = 2 * max(len(vals) for vals in sorted_coords)

    for i in range(n_filtered):
        if point_status[i] != -1:
            continue
        for k in range(d):
            Z[i, k] = model.addVar(vtype=GRB.BINARY, name=f"Z_{i}_{k}")

            r_ik = filtered_point_ranks[i][k]
            model.addConstr(r_ik >= pos_L[k] - M * (1 - Z[i, k]))
            model.addConstr(r_ik <= pos_R[k] + M * (1 - Z[i, k]))

            y_L_ik = model.addVar(vtype=GRB.BINARY, name=f"yL_{i}_{k}")
            y_R_ik = model.addVar(vtype=GRB.BINARY, name=f"yR_{i}_{k}")

            model.addConstr(pos_L[k] >= r_ik + 1 - M * (1 - y_L_ik))
            model.addConstr(pos_R[k] <= r_ik - 1 + M * (1 - y_R_ik))
            model.addConstr(y_L_ik + y_R_ik + Z[i, k] >= 1)

            model.addConstr(O[i] <= Z[i, k])

        model.addConstr(O[i] >= gp.quicksum(Z[i, k] for k in range(d)) - (d - 1))

    # --- Similarity bookkeeping ---
    S_ino = gp.LinExpr()
    S_io = gp.LinExpr()
    S_oi = gp.LinExpr()

    for i in range(n_filtered):
        p = filtered_points[i]
        if p['in_I']:
            S_ino += O[i]
            S_io += (1 - O[i])
        else:
            S_oi += O[i]

    # --- Multi-color fairness (nC2 pairwise constraints) ---
    unique_colors = sorted(set(p['color_id'] for p in points))
    if color_weights is None:
        color_weights = {c: 1.0 for c in unique_colors}
    color_counts = {c: gp.LinExpr() for c in unique_colors}
    for i in range(n_filtered):
        color_counts[filtered_points[i]['color_id']] += O[i]
    for idx_a, ca in enumerate(unique_colors):
        for cb in unique_colors[idx_a + 1:]:
            rule = "difference"  # default
            if fairness_rules is not None:
                if (ca, cb) in fairness_rules:
                    rule = fairness_rules[(ca, cb)]
                elif (cb, ca) in fairness_rules:
                    rule = fairness_rules[(cb, ca)]
                else:
                    rule = "none"
            elif diff_matrix is not None or ratio_matrix is not None:
                rule = "none"

            W_a_C_a = color_weights.get(ca, 1.0) * color_counts[ca]
            W_b_C_b = color_weights.get(cb, 1.0) * color_counts[cb]
            
            applied_custom = False
            if diff_matrix is not None:
                d_limit = diff_matrix.get((ca, cb))
                if d_limit is None: d_limit = diff_matrix.get((cb, ca))
                if d_limit is not None:
                    model.addConstr(W_a_C_a - W_b_C_b <= d_limit)
                    model.addConstr(W_a_C_a - W_b_C_b >= -d_limit)
                    applied_custom = True
            
            if ratio_matrix is not None:
                r_limit = ratio_matrix.get((ca, cb))
                if r_limit is None: r_limit = ratio_matrix.get((cb, ca))
                if r_limit is not None:
                    model.addConstr(W_b_C_b - (1.0 + r_limit) * W_a_C_a <= 0)
                    model.addConstr(W_a_C_a - (1.0 + r_limit) * W_b_C_b <= 0)
                    applied_custom = True

            if not applied_custom:
                if rule == "none":
                    continue
                if rule == "difference":
                    model.addConstr(W_a_C_a - W_b_C_b <= epsilon)
                    model.addConstr(W_a_C_a - W_b_C_b >= -epsilon)
                elif rule == "ratio":
                    model.addConstr(W_b_C_b - (1.0 + epsilon) * W_a_C_a <= 0)
                    model.addConstr(W_a_C_a - (1.0 + epsilon) * W_b_C_b <= 0)

    # --- Multi-color coverage constraints ---
    if coverage:
        for c in unique_colors:
            min_val = 1
            if min_coverage is not None:
                if isinstance(min_coverage, dict):
                    min_val = min_coverage.get(c, 0)
                elif isinstance(min_coverage, (list, tuple)):
                    if 0 <= c < len(min_coverage):
                        min_val = min_coverage[c]
                    else:
                        min_val = 0
            model.addConstr(color_counts[c] >= min_val)

    return model, S_ino, S_io, S_oi, l_vars, r_vars, sorted_coords, allowed_L_vars, allowed_R_vars


def solve_fair_range_binary_search_warm_start(points, input_box_indices, qbox_bounds, alpha, beta, epsilon, d=2, delta_base=2, use_dynamic_grid=True, start_low=0.0, warm_start_box=None, K_add=None, K_drop=None, color_weights=None, coverage=False, min_coverage=None, fairness_rules=None, diff_matrix=None, ratio_matrix=None):
    low = start_low
    high = 1.0
    best_gamma = start_low
    best_box = warm_start_box

    if DEBUG:
        print(f"Building ILP Model once for Warm Start... (Dynamic Grid: {use_dynamic_grid})")
    model, S_ino, S_io, S_oi, l_vars, r_vars, sorted_coords, allowed_L_vars, allowed_R_vars = build_model(
        points, input_box_indices, qbox_bounds, epsilon,
        d, delta_base, use_dynamic_grid, warm_start_box, K_add, K_drop, color_weights=color_weights, coverage=coverage, min_coverage=min_coverage, fairness_rules=fairness_rules, diff_matrix=diff_matrix, ratio_matrix=ratio_matrix)

    similarity_constraint = None

    if DEBUG:
        print(f"Starting Binary Search for Gamma... (Starting at low={low:.4f})")
    while high - low > 0.00005:
        mid = (low + high) / 2

        if similarity_constraint is not None:
            model.remove(similarity_constraint)

        lhs = (1 - mid) * S_ino - mid * alpha * S_io - mid * beta * S_oi
        similarity_constraint = model.addConstr(lhs >= 0, name="SimilarityConstraint")

        model.setObjective(0, GRB.MAXIMIZE)

        if best_box is not None:
            for k in range(d):
                for j in allowed_L_vars[k]:
                    l_vars[k][j].Start = GRB.UNDEFINED
                for j in allowed_R_vars[k]:
                    r_vars[k][j].Start = GRB.UNDEFINED

                l_val = best_box[k][0]
                r_val = best_box[k][1]

                if l_val in sorted_coords[k]:
                    l_idx = sorted_coords[k].index(l_val)
                    if l_idx in l_vars[k]:
                        l_vars[k][l_idx].Start = 1

                if r_val in sorted_coords[k]:
                    r_idx = sorted_coords[k].index(r_val)
                    if r_idx in r_vars[k]:
                        r_vars[k][r_idx].Start = 1

        model.optimize()

        if model.Status == GRB.OPTIMAL:
            new_box = []
            for k in range(d):
                l_idx = -1
                r_idx = -1
                for j in allowed_L_vars[k]:
                    if l_vars[k][j].X > 0.5:
                        l_idx = j
                for j in allowed_R_vars[k]:
                    if r_vars[k][j].X > 0.5:
                        r_idx = j

                if l_idx == -1: l_idx = 0
                if r_idx == -1: r_idx = len(sorted_coords[k]) - 1

                new_box.append((sorted_coords[k][l_idx], sorted_coords[k][r_idx]))

            best_box = new_box

            output_indices = set()
            for i, p in enumerate(points):
                in_box = True
                for k in range(d):
                    if not (new_box[k][0] <= p['coords'][k] <= new_box[k][1]):
                        in_box = False
                        break
                if in_box:
                    output_indices.add(i)

            intersection_count = len(input_box_indices.intersection(output_indices))
            union_count = len(input_box_indices.union(output_indices))
            actual_gamma = intersection_count / union_count if union_count > 0 else 0.0

            best_gamma = max(mid, actual_gamma)
            low = best_gamma
        else:
            high = mid

    return best_gamma, best_box

def read_input_file(filename="points.txt"):
    try:
        with open(filename, 'r') as f:
            tokens = f.read().split()
    except FileNotFoundError:
        print(f"Error: File {filename} not found.")
        return None, None, None, None

    iterator = iter(tokens)
    try:
        n = int(next(iterator))
        d = int(next(iterator))
        t = int(next(iterator))

        points = []
        for i in range(n):
            coords = []
            for _ in range(d + t):
                coords.append(float(next(iterator)))
            points.append({'coords': coords, 'index': i})

        for i in range(n):
            points[i]['color_id'] = int(next(iterator))

        qbox_bounds = []
        for _ in range(d):
            min_v = float(next(iterator))
            max_v = float(next(iterator))
            qbox_bounds.append((min_v, max_v))

        return points, d, t, qbox_bounds

    except StopIteration:
        print("Error: Unexpected end of file.")
        return None, None, None, None
    except ValueError:
        print("Error: Invalid number format.")
        return None, None, None, None

if __name__ == "__main__":
    filename = "points.txt"
    constraints_file = "constraints.txt"
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    if len(sys.argv) > 2:
        constraints_file = sys.argv[2]

    print(f"Reading data from {filename}...")
    points, d, t, qbox = read_input_file(filename)

    if points is None:
        sys.exit(1)

    print(f"Loaded {len(points)} points. Dimensions: d={d}, t={t}.")
    
    from parse_constraints import apply_parsed_constraints
    p_color_weights, p_coverage, p_min_coverage, diff_matrix, ratio_matrix = apply_parsed_constraints(constraints_file)
    print("Solver received constraints:")
    print(f"  Weights: {p_color_weights}")
    print(f"  Coverage: {p_coverage}")
    print(f"  Min Coverage: {p_min_coverage}")
    print(f"  Diff Matrix: {diff_matrix}")
    print(f"  Ratio Matrix: {ratio_matrix}")

    input_indices = set()
    for i, p in enumerate(points):
        in_box = True
        for k in range(d):
            if not (qbox[k][0] <= p['coords'][k] <= qbox[k][1]):
                in_box = False
                break
        if in_box:
            input_indices.add(i)

    print(f"Query Box I contains {len(input_indices)} points.")

    alpha = 1.0
    beta = 1.0
    epsilon = 0.0
    delta_base = 2

    coverage = p_coverage
    min_coverage = p_min_coverage
    color_weights = p_color_weights

    print(f"Solving ILP with Optimized Dynamic Grid Binary Search Warm Start Bounded (alpha={alpha}, beta={beta}, epsilon={epsilon}, delta={delta_base})...")

    start_time = time.time()
    try:
        gamma_approx, box_approx = solve_fair_range_binary_search_warm_start(
            points, input_indices, qbox, alpha, beta, epsilon,
            d=d, delta_base=delta_base, use_dynamic_grid=True,
            color_weights=color_weights, coverage=coverage, min_coverage=min_coverage,
            diff_matrix=diff_matrix, ratio_matrix=ratio_matrix)

        if box_approx is not None:
            if gamma_approx > 0:
                K_drop = int(len(input_indices) * (1 - gamma_approx))
                K_add  = int(len(input_indices) * ((1.0 / gamma_approx) - 1))
            else:
                K_drop, K_add = None, None

            gamma, box = solve_fair_range_binary_search_warm_start(
                points, input_indices, qbox, alpha, beta, epsilon,
                d=d, delta_base=1, use_dynamic_grid=False,
                start_low=gamma_approx, warm_start_box=box_approx,
                K_add=K_add, K_drop=K_drop,
                color_weights=color_weights, coverage=coverage, min_coverage=min_coverage,
                diff_matrix=diff_matrix, ratio_matrix=ratio_matrix)
        else:
            gamma, box = solve_fair_range_binary_search_warm_start(
                points, input_indices, qbox, alpha, beta, epsilon,
                d=d, delta_base=1, use_dynamic_grid=False,
                color_weights=color_weights, coverage=coverage, min_coverage=min_coverage,
                diff_matrix=diff_matrix, ratio_matrix=ratio_matrix)
    except:
        traceback.print_exc()
        gamma = 0
        box = None

    end_time = time.time()

    print(f"Time taken to find fair range: {end_time - start_time:.4f} seconds")
    print(f"Optimal Gamma: {gamma:.4f}")

    if box:
        print("Optimal Box Bounds:")
        for k in range(d):
            print(f"  Dim {k}: [{box[k][0]:.6f}, {box[k][1]:.6f}]")

        output_indices = set()
        for i, p in enumerate(points):
            in_box = True
            for k in range(d):
                if not (box[k][0] <= p['coords'][k] <= box[k][1]):
                    in_box = False
                    break
            if in_box:
                output_indices.add(i)

        intersection_count = len(input_indices.intersection(output_indices))
        union_count = len(input_indices.union(output_indices))

        jaccard = intersection_count / union_count if union_count > 0 else 0.0
        print(f"Jaccard Similarity: {jaccard:.4f}")
    else:
        print("No feasible box found.")
