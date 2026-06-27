import os
import random
import re
import subprocess
import sys
import time
import numpy as np
import copy

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
        
    return n, d, lines[0:3+n], coords, colors

def generate_queries(coords, n, d, num_per_bucket=5):
    buckets = {
        'b1': {'min_pts': int(0.20 * n), 'max_pts': int(0.40 * n), 'queries': []},
        'b2': {'min_pts': int(0.40 * n), 'max_pts': int(0.60 * n), 'queries': []},
        'b3': {'min_pts': int(0.60 * n), 'max_pts': int(0.80 * n), 'queries': []},
        'b4': {'min_pts': int(0.80 * n), 'max_pts': int(1.00 * n), 'queries': []},
    }
    
    coords_np = np.array(coords)
    
    print("Generating queries for buckets...")
    attempts = 0
    while True:
        attempts += 1
        qbox = []
        for k in range(d):
            vals = coords_np[:, k]
            p_low = random.uniform(0, 40)
            p_high = random.uniform(60, 100)
            
            if random.random() < 0.2:
                p_low = random.uniform(0, 10)
                p_high = random.uniform(90, 100)
                
            min_v = round(float(np.percentile(vals, p_low)), 6)
            max_v = round(float(np.percentile(vals, p_high)), 6)
            qbox.append((min_v, max_v))
            
        in_box = 0
        input_indices = set()
        for i, p in enumerate(coords):
            if all(qbox[k][0] <= p[k] <= qbox[k][1] for k in range(d)):
                in_box += 1
                input_indices.add(i)
                
        for b_name, b_info in buckets.items():
            if b_info['min_pts'] <= in_box <= b_info['max_pts']:
                if len(b_info['queries']) < num_per_bucket:
                    b_info['queries'].append((qbox, in_box, input_indices))
                    print(f"  -> Found query for {b_name} with {in_box} points ({in_box/n*100:.2f}%)")
                break
                
        done = True
        for b_info in buckets.values():
            if len(b_info['queries']) < num_per_bucket:
                done = False
                break
        if done:
            break
            
        if attempts > 10000:
            print("Took too many attempts to generate queries. Partial results collected.")
            break
            
    return buckets

def generate_dynamic_constraints(qbox, input_indices, coords, colors, d, num_colors=5):
    ranges = [qbox[k][1] - qbox[k][0] for k in range(d)]
    
    candidates = []
    attempts = 0
    
    # Try to find a target box with sim between 0.7 and 0.9
    while attempts < 2000:
        attempts += 1
        cand_box = []
        for k in range(d):
            # shift bounds slightly
            shift_min = random.uniform(-0.15, 0.15) * ranges[k]
            shift_max = random.uniform(-0.15, 0.15) * ranges[k]
            
            min_v = min(qbox[k][0] + shift_min, qbox[k][1] + shift_max)
            max_v = max(qbox[k][0] + shift_min, qbox[k][1] + shift_max)
            cand_box.append((min_v, max_v))
            
        cand_indices = set()
        for i, p in enumerate(coords):
            if all(cand_box[k][0] <= p[k] <= cand_box[k][1] for k in range(d)):
                cand_indices.add(i)
                
        intersection = len(input_indices & cand_indices)
        union = len(input_indices | cand_indices)
        sim = intersection / union if union > 0 else 0
        
        candidates.append((cand_box, cand_indices, sim))
        if 0.70 <= sim <= 0.90:
            break
            
    # Find candidates in [0.70, 0.90]
    in_range = [c for c in candidates if 0.70 <= c[2] <= 0.90]
    if in_range:
        # Pick the one closest to 0.8
        in_range.sort(key=lambda x: abs(x[2] - 0.80))
        target_box, target_indices, final_sim = in_range[0]
        print(f"    -> Successfully found target box with sim: {final_sim:.4f}")
    else:
        # Sort all by proximity to 0.8
        candidates.sort(key=lambda x: abs(x[2] - 0.80))
        target_box, target_indices, final_sim = candidates[0]
        print(f"    -> Warning: Could not find target box with sim in [0.7, 0.9]. Using closest sim: {final_sim:.4f}")
        
    target_counts = {c: 0 for c in range(1, num_colors+1)}
    for idx in target_indices:
        target_counts[colors[idx]] += 1
        
    # Generate 8-9 constraints total
    cov_arr = ["0"] * num_colors
    cov_colors = random.sample(range(1, num_colors+1), 3)
    for c in cov_colors:
        cov_arr[c-1] = str(max(0, target_counts[c]))
        
    diff_matrix = [["NaN"] * num_colors for _ in range(num_colors)]
    pairs = [(i, j) for i in range(1, num_colors+1) for j in range(i+1, num_colors+1)]
    diff_pairs = random.sample(pairs, 3)
    for ca, cb in diff_pairs:
        val = abs(target_counts[ca] - target_counts[cb])
        diff_matrix[ca-1][cb-1] = str(val)
        # Leave cb-1, ca-1 as NaN
        
    ratio_matrix = [["NaN"] * num_colors for _ in range(num_colors)]
    remaining_pairs = [p for p in pairs if p not in diff_pairs]
    ratio_pairs = random.sample(remaining_pairs, 3) if len(remaining_pairs) >= 3 else remaining_pairs
    for ca, cb in ratio_pairs:
        ta, tb = target_counts[ca], target_counts[cb]
        if ta > 0 and tb > 0:
            r = round(max(ta/tb, tb/ta), 4)
            ratio_matrix[ca-1][cb-1] = str(r)
            # Leave cb-1, ca-1 as NaN
            
    content = "Weights of color:\n"
    content += " ".join(["1.0"] * num_colors) + "\n\n"
    
    content += "Coverage:\n"
    content += " ".join(cov_arr) + "\n\n"
    
    content += "Difference:\n"
    for i in range(num_colors):
        content += " ".join(diff_matrix[i]) + "\n"
    content += "\n"
    
    content += "Ratio:\n"
    for i in range(num_colors):
        content += " ".join(ratio_matrix[i]) + "\n"
        
    return content

def run_experiment():
    dataset_file = "NYC_30k_5colors.txt"
    temp_data_file = "temp_experiment_data.txt"
    temp_constraints = "temp_constraints_ablation.txt"
    log_file = "ablation_study_log.txt"
    
    if not os.path.exists("bfsmp"):
        print("Compiling bfsmp.cpp...")
        subprocess.run(["g++", "-O3", "bfsmp.cpp", "-o", "bfsmp"])
        
    print(f"Loading {dataset_file}...")
    n, d, header_lines, coords, colors = load_base_data(dataset_file)
    
    buckets = generate_queries(coords, n, d, num_per_bucket=5)
    
    solvers = [
        {"name": "Dinkelbach_WS_Grid", "cmd": ["python3", "solve_ilp_dynamic_grid_dinkelbach_warm_start_bounded_optimized_fast.py"]},
        {"name": "Dinkelbach_NoWS", "cmd": ["python3", "solve_ilp_Dinkleback_gamma_bounded.py"]},
        {"name": "BinarySearch_NoGrid", "cmd": ["python3", "solve_ilp_binary_search_gamma_bounded.py"]},
        {"name": "BinarySearch_WS_Grid", "cmd": ["python3", "solve_ilp_dynamic_grid_binary_search_warm_start_bounded_optimized.py"]},
        {"name": "BFSMP", "cmd": ["./bfsmp"]}
    ]
    
    with open(log_file, "w") as f:
        f.write("="*100 + "\n")
        f.write("ABLATION STUDY EXPERIMENT\n")
        f.write("="*100 + "\n\n")
        
    results = {b: {s["name"]: {"times": [], "sims": []} for s in solvers} for b in buckets}
    
    query_count = 1
    total_queries = sum(len(b_info['queries']) for b_info in buckets.values())
        
    for b_name, b_info in buckets.items():
        print(f"\n=========================================")
        print(f"STARTING BUCKET: {b_name} (Range: {b_info['min_pts']}-{b_info['max_pts']})")
        print(f"=========================================")
        
        for q_idx, (qbox, in_box, input_indices) in enumerate(b_info['queries']):
            print(f"\n--- Query {query_count}/{total_queries} | Bucket: {b_name} | Points: {in_box} ({in_box/n*100:.2f}%) ---")
            query_count += 1
            
            # Generate dynamic constraints for this specific query box with similarity in range [0.7, 0.9]
            constraint_str = generate_dynamic_constraints(qbox, input_indices, coords, colors, d)
            with open(temp_constraints, 'w') as f:
                f.write(constraint_str)
                
            with open(temp_data_file, 'w') as f:
                for line in header_lines:
                    f.write(line)
                for min_v, max_v in qbox:
                    f.write(f"{min_v} {max_v}\n")
                    
            for solver in solvers:
                print(f"  -> Running {solver['name']}...")
                with open(temp_constraints, 'r') as fc:
                    print(f"     [DEBUG] Constraints being passed to {solver['name']}:\n{fc.read().strip()}\n")
                cmd = solver['cmd'] + [temp_data_file, temp_constraints]
                
                start_time = time.time()
                time_taken = "N/A"
                jaccard = "N/A"
                
                try:
                    res = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                    output_lines = []
                    for line in iter(res.stdout.readline, ''):
                        print(line, end='')
                        sys.stdout.flush()
                        output_lines.append(line)
                    res.stdout.close()
                    return_code = res.wait(timeout=3600)
                    output = "".join(output_lines)
                    
                    if solver['name'] == "BFSMP":
                        time_match = re.search(r"Execution time:\s*([0-9.eE+-]+)\s*seconds", output)
                        sim_match = re.search(r"Max sim:\s*([0-9.eE+-]+)", output)
                        
                        if time_match: time_taken = float(time_match.group(1))
                        if sim_match: jaccard = float(sim_match.group(1))
                            
                        if "No feasible box found" in output: jaccard = "Infeasible"
                    else:
                        time_match = re.search(r"Time taken to find fair range:\s*([0-9.]+)\s*seconds", output)
                        sim_match = re.search(r"Jaccard Similarity:\s*([0-9.]+)", output)
                        
                        if time_match: time_taken = float(time_match.group(1))
                        if sim_match: jaccard = float(sim_match.group(1))
                            
                        if "No feasible box found" in output: jaccard = "Infeasible"
                            
                    if return_code != 0:
                        jaccard = "ERROR"
                        time_taken = "ERROR"
                        
                except subprocess.TimeoutExpired:
                    time_taken = "TIMEOUT"
                    jaccard = "TIMEOUT"
                except Exception as e:
                    time_taken = "FAIL"
                    jaccard = "FAIL"
                    
                print(f"     Result: Time = {time_taken}, Sim = {jaccard}")
                
                results[b_name][solver['name']]['times'].append(time_taken)
                results[b_name][solver['name']]['sims'].append(jaccard)
                
                with open(log_file, "a") as f:
                    f.write(f"Bucket: {b_name} | Query {q_idx+1} | Points: {in_box}\n")
                    f.write(f"Solver: {solver['name']}\n")
                    f.write(f"Time: {time_taken} | Sim: {jaccard}\n")
                    f.write("-" * 50 + "\n")
                    
    print("\n\n" + "="*80)
    print("ABLATION STUDY SUMMARY")
    print("="*80)
    
    with open(log_file, "a") as f:
        f.write("\n\n" + "="*80 + "\n")
        f.write("ABLATION STUDY SUMMARY\n")
        f.write("="*80 + "\n")
        
    for b_name in buckets.keys():
        print(f"\nBucket: {b_name}")
        with open(log_file, "a") as f:
            f.write(f"\nBucket: {b_name}\n")
            
        header = f"{'Solver':<25} | {'Avg Time (s)':<15} | {'Avg Sim':<15} | {'Successes'}"
        print(header)
        print("-" * len(header))
        with open(log_file, "a") as f:
            f.write(header + "\n")
            f.write("-" * len(header) + "\n")
            
        for solver in solvers:
            s_name = solver['name']
            times = results[b_name][s_name]['times']
            sims = results[b_name][s_name]['sims']
            
            valid_times = [t for t in times if isinstance(t, (int, float))]
            valid_sims = [s for s in sims if isinstance(s, (int, float))]
            
            avg_time = sum(valid_times)/len(valid_times) if valid_times else 0.0
            avg_sim = sum(valid_sims)/len(valid_sims) if valid_sims else 0.0
            successes = len(valid_sims)
            
            row = f"{s_name:<25} | {avg_time:<15.4f} | {avg_sim:<15.4f} | {successes}/5"
            print(row)
            with open(log_file, "a") as f:
                f.write(row + "\n")
                
    if os.path.exists(temp_data_file): os.remove(temp_data_file)
    if os.path.exists(temp_constraints): os.remove(temp_constraints)

if __name__ == '__main__':
    run_experiment()
