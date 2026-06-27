import gurobipy as gp
from gurobipy import GRB
import time
import sys
import traceback

DEBUG = False

def build_model(points, input_box_indices, qbox, epsilon, d=2, color_weights=None, coverage=False, min_coverage=None, fairness_rules=None, diff_matrix=None, ratio_matrix=None):
    n = len(points)
    
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

    # --- Model ---
    model = gp.Model("FairRangeFeasibility")
    model.setParam("OutputFlag", 0)

    # --- Boundary variables ---
    l_vars, r_vars, pos_L, pos_R = [], [], [], []
    for k in range(d):
        m = len(sorted_coords[k])

        max_l_idx = m - 1
        for j in range(m - 1, -1, -1):
            if sorted_coords[k][j] <= qbox[k][1]:
                max_l_idx = j
                break
                
        min_r_idx = 0
        for j in range(m):
            if sorted_coords[k][j] >= qbox[k][0]:
                min_r_idx = j
                break

        valid_l_indices = list(range(max_l_idx + 1))
        valid_r_indices = list(range(min_r_idx, m))
        
        l = model.addVars(valid_l_indices, vtype=GRB.BINARY, name=f"L_{k}")
        r = model.addVars(valid_r_indices, vtype=GRB.BINARY, name=f"R_{k}")
        
        l_vars.append(l)
        r_vars.append(r)

        model.addConstr(l.sum() == 1)
        model.addConstr(r.sum() == 1)

        posL_expr = gp.quicksum(j * l[j] for j in valid_l_indices)
        posR_expr = gp.quicksum(j * r[j] for j in valid_r_indices)
        
        posL_var = model.addVar(vtype=GRB.INTEGER, name=f"posL_val_{k}", ub=max_l_idx)
        posR_var = model.addVar(vtype=GRB.INTEGER, name=f"posR_val_{k}", lb=min_r_idx)
        
        model.addConstr(posL_var == posL_expr)
        model.addConstr(posR_var == posR_expr)
        model.addConstr(posL_var <= posR_var)

        pos_L.append(posL_var)
        pos_R.append(posR_var)

    # --- Output indicator ---
    O = model.addVars(n, vtype=GRB.BINARY, name="O")

    # --- Dimension satisfaction indicators ---
    Z = {}
    M = 2*max(len(vals) for vals in sorted_coords)

    for i in range(n):
        for k in range(d):
            Z[i, k] = model.addVar(vtype=GRB.BINARY, name=f"Z_{i}_{k}")

            r_ik = point_ranks[i][k]
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

    for i in range(n):
        in_I = (i in input_box_indices)

        if in_I:
            S_ino += O[i]
            S_io += (1 - O[i])
        else:
            S_oi += O[i]

    # --- Multi-color fairness (nC2 pairwise constraints) ---
    unique_colors = sorted(set(p['color_id'] for p in points))
    if color_weights is None:
        color_weights = {c: 1.0 for c in unique_colors}
    color_counts = {c: gp.LinExpr() for c in unique_colors}
    for i in range(n):
        color_counts[points[i]['color_id']] += O[i]
    for idx_a, ca in enumerate(unique_colors):
        for cb in unique_colors[idx_a + 1:]:
            # Get the rule for this pair, checking both (ca, cb) and (cb, ca)
            rule = "difference"  # default
            if fairness_rules is not None:
                if (ca, cb) in fairness_rules:
                    rule = fairness_rules[(ca, cb)]
                elif (cb, ca) in fairness_rules:
                    rule = fairness_rules[(cb, ca)]
                else:
                    rule = "none" # if rules exist but this pair is not specified, default to none
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
    
    return model, S_ino, S_io, S_oi, l_vars, r_vars, sorted_coords


def solve_fair_range_Dinkleback_warm_start(points, input_box_indices, qbox, alpha, beta, epsilon, d=2, color_weights=None, coverage=False, min_coverage=None, fairness_rules=None, diff_matrix=None, ratio_matrix=None):
    """
    Performs Dinkleback search on gamma to find the maximum gamma for which a feasible box exists.
    Uses warm-starting by maintaining the Gurobi model across iterations.
    """
    feasible_gamma= 0.0
    previous_gamma = -1.0

    if DEBUG:
        print("Building ILP Model once for Warm Start...")
    model, S_ino, S_io, S_oi, l_vars, r_vars, sorted_coords = build_model(
        points, input_box_indices, qbox, epsilon, d, color_weights=color_weights, coverage=coverage, min_coverage=min_coverage, fairness_rules=fairness_rules, diff_matrix=diff_matrix, ratio_matrix=ratio_matrix)

    if DEBUG:
        print("Starting Dinkelbach Search for Gamma...")
    if DEBUG:
        print(f"Gamma : {feasible_gamma}")
    
    best_box = None
    
    while abs(feasible_gamma - previous_gamma) > 0.005: 
        # Update objective on the SAME model
        model.setObjective((1 - feasible_gamma) * S_ino - feasible_gamma * alpha * S_io - feasible_gamma * beta * S_oi, GRB.MAXIMIZE)
        model.optimize()
        
        if model.Status == GRB.OPTIMAL:
            # Extract solution
            best_box = []
            for k in range(d):
                l_idx = -1
                r_idx = -1
                for j in l_vars[k].keys():
                    if l_vars[k][j].X > 0.5:
                        l_idx = j
                for j in r_vars[k].keys():
                    if r_vars[k][j].X > 0.5:
                        r_idx = j
                
                if l_idx == -1: l_idx = 0
                if r_idx == -1: r_idx = len(sorted_coords[k]) - 1
                
                best_box.append((sorted_coords[k][l_idx], sorted_coords[k][r_idx]))
                
            inter = S_ino.getValue()
            inoto = S_io.getValue()
            onoti = S_oi.getValue()
            
            previous_gamma = feasible_gamma
            
            # Prevent division by zero if optimal box is empty
            denom = inter + alpha * inoto + beta * onoti
            if denom == 0:
                feasible_gamma = 0.0
                break
                
            feasible_gamma = inter / denom
            if DEBUG:
                print(f"Gamma {feasible_gamma:.4f} is FEASIBLE.")
        else:
            if DEBUG:
                print(f"Something went wrong in Dinkleback optimization!\nINFEASIBLE.")
            return feasible_gamma, best_box
            
    return feasible_gamma, best_box

# --- File Parsing ---
def read_input_file(filename="points.txt"):
    """
    Reads input in the format specified by bfsmp_fread_copy.cpp
    """
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
        # Read Points Coords
        for i in range(n):
            coords = []
            for _ in range(d + t):
                coords.append(float(next(iterator)))
            points.append({'coords': coords, 'index': i})
            
        # Read Colors
        for i in range(n):
            points[i]['color_id'] = int(next(iterator))
            
        # Read Query Box
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

# --- Main Block ---
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
    
    coverage = p_coverage
    min_coverage = p_min_coverage
    color_weights = p_color_weights

    print(f"Solving ILP with Dinkleback Warm Start (alpha={alpha}, beta={beta}, epsilon={epsilon})...")
    
    start_time = time.time()
    try:
        gamma, box = solve_fair_range_Dinkleback_warm_start(
            points, input_indices, qbox, alpha, beta, epsilon, d=d,
            color_weights=color_weights, coverage=coverage, min_coverage=min_coverage,
            diff_matrix=diff_matrix, ratio_matrix=ratio_matrix
        )
    except:
        traceback.print_exc()
        gamma = 0
        box = None
        
    end_time = time.time()
    
    print(f"Time taken to find fair range: {end_time - start_time:.4f} seconds")
    print(f"Optimal Gamma (Dinkleback Warm Start Bounded): {gamma:.4f}")
    
    if box:
        print("Optimal Box Bounds:")
        for k in range(d):
            print(f"  Dim {k}: [{box[k][0]:.6f}, {box[k][1]:.6f}]")

        # --- Jaccard Similarity Calculation ---
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
