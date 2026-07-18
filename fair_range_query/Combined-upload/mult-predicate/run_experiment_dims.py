import os
import random
import re
import subprocess
import sys
import time
import numpy as np

def load_base_data(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()
        
    n = int(lines[0].strip())
    d_t = lines[1].strip()
    d = int(d_t.split()[0])
    
    # Coordinates start at line 2 and end at line 2+n
    coords_lines = lines[2:2+n]
    colors_line = lines[2+n].strip()
    colors = list(map(int, colors_line.split()))
    
    coords = []
    for line in coords_lines:
        coords.append(list(map(float, line.strip().split())))
        
    return n, d, coords, colors

def check_if_fair(input_indices, colors, in_box, max_pts):
    counts = {1: 0, 2: 0, 3: 0, 4: 0, 5: 0}
    for idx in input_indices:
        c = colors[idx]
        if c in counts:
            counts[c] += 1
            
    # Check if fair for Coverage (most relaxed theta = 0.8)
    cov_threshold = int(0.8 * (max_pts / 5.0))
    fair_coverage = all(counts[c] >= cov_threshold for c in counts)
            
    # Check if fair for Difference (most relaxed bound eps = 120)
    fair_diff = True
    if abs(counts[1] - counts[3]) > 120.0: fair_diff = False
    if abs(counts[3] - counts[4]) > 120.0: fair_diff = False
    if abs(counts[3] - counts[5]) > 120.0: fair_diff = False
    
    # Check if fair for Ratio (most relaxed bound delta = 0.06)
    fair_ratio = True
    if counts[4] - 1.03 * counts[1] > 1e-6: fair_ratio = False
    if counts[1] - 1.03 * counts[4] > 1e-6: fair_ratio = False
    if counts[5] - 1.03 * counts[1] > 1e-6: fair_ratio = False
    if counts[1] - 1.03 * counts[5] > 1e-6: fair_ratio = False
    if counts[4] - 1.03 * counts[5] > 1e-6: fair_ratio = False
    if counts[5] - 1.03 * counts[4] > 1e-6: fair_ratio = False
    
    # We want the initial box to be UNFAIR for ALL criteria, so the solver has to optimize.
    return fair_coverage or fair_diff or fair_ratio

def generate_queries(coords, n, d, colors, num_per_bucket=3):
    buckets = {
        'b1': {'min_pts': int(0.20 * n), 'max_pts': int(0.40 * n), 'queries': []},
        'b2': {'min_pts': int(0.40 * n), 'max_pts': int(0.60 * n), 'queries': []},
        'b3': {'min_pts': int(0.60 * n), 'max_pts': int(0.80 * n), 'queries': []},
        'b4': {'min_pts': int(0.80 * n), 'max_pts': int(1.00 * n), 'queries': []},
    }
    
    coords_np = np.array(coords)
    
    for b_name, b_info in buckets.items():
        print(f"Generating queries for {b_name}...")
        attempts = 0
        
        target_fraction_min = b_info['min_pts'] / n
        target_fraction_max = b_info['max_pts'] / n
        
        while len(b_info['queries']) < num_per_bucket:
            attempts += 1
            if attempts > 100000:
                print(f"Took too many attempts for {b_name}. Moving on.")
                break
                
            target_frac = random.uniform(target_fraction_min, target_fraction_max)
            base_width = (target_frac ** (1.0 / d)) * 100.0
            
            qbox = []
            for k in range(d):
                vals = coords_np[:, k]
                
                # Add some variation to the width while targeting the base_width
                width = min(100.0, max(1.0, random.gauss(base_width, 10.0)))
                p_low = random.uniform(0.0, 100.0 - width)
                p_high = p_low + width
                
                min_v = round(float(np.percentile(vals, p_low)), 6)
                max_v = round(float(np.percentile(vals, p_high)), 6)
                qbox.append((min_v, max_v))
                
            qbox_np = np.array(qbox)
            mins = qbox_np[:, 0]
            maxs = qbox_np[:, 1]
            
            in_bounds = np.all((coords_np >= mins) & (coords_np <= maxs), axis=1)
            in_box = int(np.sum(in_bounds))
            
            if not (b_info['min_pts'] <= in_box <= b_info['max_pts']):
                continue
                
            input_indices = set(np.where(in_bounds)[0].tolist())
                
            is_fair = check_if_fair(input_indices, colors, in_box, b_info['max_pts'])
            if is_fair:
                continue
                
            b_info['queries'].append((qbox, in_box, input_indices))
            print(f"  -> Found query for {b_name} with {in_box} points ({in_box/n*100:.2f}%) at attempt {attempts}")
            
    return buckets


def write_dataset_file(filename, n, d, coords, colors, qbox=None):
    with open(filename, 'w') as f:
        f.write(f"{n}\n")
        f.write(f"{d} 0\n")
        
        for p in coords:
            row_str = " ".join(f"{val:.6f}" for val in p)
            f.write(f"{row_str}\n")
            
        colors_str = " ".join(str(c) for c in colors)
        f.write(f"{colors_str}\n")
        
        if qbox is not None:
            for min_v, max_v in qbox:
                f.write(f"{min_v} {max_v}\n")

def run_experiment():
    dataset_file = "NYC_30k_5colors.txt"
    log_file = "experiment_dims_log_nyc.txt"
    
    if not os.path.exists("bfsmp"):
        print("Compiling bfsmp.cpp...")
        subprocess.run(["g++", "-O3", "bfsmp.cpp", "-o", "bfsmp"])
        
    print(f"Loading {dataset_file}...")
    n, d_full, coords_full, colors = load_base_data(dataset_file)
    
    solvers = [
        {"name": "Dinkelbach_WS_Grid", "cmd": ["python3", "solve_ilp_dynamic_grid_dinkelbach_warm_start_bounded_optimized_copy.py"]},
        {"name": "BFSMP", "cmd": ["./bfsmp"]}
    ]
    
    with open(log_file, "w") as f:
        f.write("="*100 + "\n")
        f.write("DIMENSIONALITY ABLATION STUDY EXPERIMENT (NYC)\n")
        f.write("="*100 + "\n\n")
        
    for d_current in range(2, 6):
        temp_data_file = f"temp_experiment_data_d{d_current}.txt"
        temp_constraints = f"temp_constraints_ablation_d{d_current}.txt"
        
        print(f"\\n" + "#"*50)
        print(f"STARTING EXPERIMENT FOR d = {d_current}")
        print("#"*50)
        
        # Extract the first d_current coordinates
        coords_d = [c[:d_current] for c in coords_full]
        
        buckets = generate_queries(coords_d, n, d_current, colors, num_per_bucket=1)
        
        results = {b: {s["name"]: {"times": [], "sims": []} for s in solvers} for b in buckets}
        
        query_count = 1
        total_queries = sum(len(b_info['queries']) for b_info in buckets.values())
        
        with open(log_file, "a") as f:
            f.write(f"\\n==================== d = {d_current} ====================\\n")
            
        for b_name, b_info in buckets.items():
            print(f"\\n=========================================")
            print(f"STARTING BUCKET: {b_name} (Range: {b_info['min_pts']}-{b_info['max_pts']})")
            print(f"=========================================")
            
            for q_idx, (qbox, in_box, input_indices) in enumerate(b_info['queries']):
                print(f"\\n--- Query {query_count}/{total_queries} | Bucket: {b_name} | Points: {in_box} ({in_box/n*100:.2f}%) ---")
                query_count += 1
                
                # Calculate and print all pair diffs and ratios for this query
                counts = {1: 0, 2: 0, 3: 0, 4: 0, 5: 0}
                for idx in input_indices:
                    c = colors[idx]
                    if c in counts:
                        counts[c] += 1
                        
                r14 = abs(counts[1]/counts[4] - 1.0) if counts[4] > 0 else 0
                r15 = abs(counts[1]/counts[5] - 1.0) if counts[5] > 0 else 0
                r45 = abs(counts[4]/counts[5] - 1.0) if counts[5] > 0 else 0
                d13 = abs(counts[1] - counts[3])
                d34 = abs(counts[3] - counts[4])
                d35 = abs(counts[3] - counts[5])
                
                print(f"Query Diffs  | 1v3: {d13}, 3v4: {d34}, 3v5: {d35}")
                print(f"Query Ratios | 1v4: {r14:.4f}, 1v5: {r15:.4f}, 4v5: {r45:.4f}")
                
                # Use dynamic constraints with 0.95 multiplier
                max_diff = max(1, d13, d34, d35)
                max_ratio = max(0.001, r14, r15, r45)
                target_eps = max_diff * 0.95
                target_delta = max_ratio * 0.95
                target_cov = int(min(counts.values()) * 0.95)
                
                print(f"Query Targets| Eps: {target_eps:.2f}, Delta: {target_delta:.4f}, Cov: {target_cov}")

                constraint_str = f"""Weights of color:
1.0 1.0 1.0 1.0 1.0

Coverage:
{target_cov} {target_cov} {target_cov} {target_cov} {target_cov}

Difference:
NaN NaN {target_eps:.2f} NaN NaN
NaN NaN NaN NaN NaN
{target_eps:.2f} NaN NaN {target_eps:.2f} {target_eps:.2f}
NaN NaN {target_eps:.2f} NaN NaN
NaN NaN {target_eps:.2f} NaN NaN

Ratio:
NaN NaN NaN {target_delta:.4f} {target_delta:.4f}
NaN NaN NaN NaN NaN
NaN NaN NaN NaN NaN
{target_delta:.4f} NaN NaN NaN {target_delta:.4f}
{target_delta:.4f} NaN NaN {target_delta:.4f} NaN
"""
                with open(temp_constraints, 'w') as f:
                    f.write(constraint_str)
                    
                write_dataset_file(temp_data_file, n, d_current, coords_d, colors, qbox)
                qbox_str = " | ".join([f"[{dim[0]:.6f}, {dim[1]:.6f}]" for dim in qbox])
                    
                for solver in solvers:
                    print(f"  -> Running {solver['name']}...")
                    cmd = solver['cmd'] + [temp_data_file, temp_constraints]
                    
                    start_time = time.time()
                    time_taken = "N/A"
                    jaccard = "N/A"
                    box_str = "N/A"
                    
                    try:
                        res = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                        output, _ = res.communicate(timeout=1800)  # 30 minutes timeout per solver run
                        return_code = res.returncode                    
                        
                        if solver['name'] == "BFSMP":
                            time_match = re.search(r"Execution time:\s*([0-9.eE+-]+)\s*seconds", output)
                            sim_match = re.search(r"Max sim:\s*([0-9.eE+-]+)", output)
                            
                            ans_size_match = re.search(r"Ans size \d+\n(.*?)\nMax sim", output, re.DOTALL)
                            if ans_size_match:
                                box_str = ans_size_match.group(1).strip()
                                
                            if time_match: time_taken = float(time_match.group(1))
                            if sim_match: jaccard = float(sim_match.group(1))
                                
                            if "No feasible box found" in output: jaccard = "Infeasible"
                        else:
                            time_match = re.search(r"Time taken to find fair range:\s*([0-9.]+)\s*seconds", output)
                            sim_match = re.search(r"Jaccard Similarity:\s*([0-9.]+)", output)
                            
                            box_match = re.search(r"Optimal Box Bounds:\n(.*?)(?=\n[A-Za-z]|$)", output, re.DOTALL)
                            if box_match:
                                lines = box_match.group(1).strip().split('\n')
                                box_str = " | ".join([line.strip() for line in lines])
                                
                            if time_match: time_taken = float(time_match.group(1))
                            if sim_match: jaccard = float(sim_match.group(1))
                                
                            if "No feasible box found" in output: jaccard = "Infeasible"
                                
                        if return_code != 0:
                            jaccard = "ERROR"
                            time_taken = "ERROR"
                            
                    except subprocess.TimeoutExpired:
                        time_taken = "TIMEOUT"
                        jaccard = "TIMEOUT"
                        try:
                            res.kill()
                        except:
                            pass
                    except Exception as e:
                        time_taken = "FAIL"
                        jaccard = "FAIL"
                        
                    print(f"     Result: Time = {time_taken}, Sim = {jaccard}")
                    print(f"     InBox  : {qbox_str}")
                    print(f"     OutBox : {box_str}")
                    
                    results[b_name][solver['name']]['times'].append(time_taken)
                    results[b_name][solver['name']]['sims'].append(jaccard)
                    
                    with open(log_file, "a") as f:
                        f.write(f"Bucket: {b_name} | Query {q_idx+1} | Points: {in_box}\\n")
                        f.write(f"Solver: {solver['name']}\\n")
                        f.write(f"Time: {time_taken} | Sim: {jaccard} | InBox: {qbox_str} | OutBox: {box_str}\\n")
                        
            # Print summary for the bucket in this dimension
            print(f"\\n--- Summary for d={d_current}, Bucket={b_name} ---")
            with open(log_file, "a") as f:
                f.write(f"\n--- Summary for d={d_current}, Bucket={b_name} ---\n")
                
                for s in solvers:
                    s_times = [t for t in results[b_name][s['name']]['times'] if isinstance(t, (int, float))]
                    s_sims = [s_val for s_val in results[b_name][s['name']]['sims'] if isinstance(s_val, (int, float))]
                    
                    avg_time = sum(s_times)/len(s_times) if s_times else 0
                    avg_sim = sum(s_sims)/len(s_sims) if s_sims else 0
                    successes = len(s_times)
                    
                    summary_str = f"Solver: {s['name']} | Avg Time: {avg_time:.4f} | Avg Sim: {avg_sim:.4f} | Success: {successes}/{len(b_info['queries'])}"
                    print(summary_str)
                    f.write(summary_str + "\n")
                f.write("\n")

if __name__ == '__main__':
    run_experiment()
