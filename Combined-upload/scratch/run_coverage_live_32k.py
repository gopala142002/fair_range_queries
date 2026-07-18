import subprocess
import random
import os
import csv
import re

def main():
    # Configuration
    dataset = "live_l2_unique_32k.csv"
    n_rows = 32000
    num_colors = "3"
    alpha = 0.5
    beta = 0.5
    
    epsilons = [0.0, 2.0 ,10.0, 100.0]
    deltas = [0, 1.0, 5.0]
    weightA = 1.0
    weightB = 1.0
    
    # Modes to test
    tree_modes = ["range_opt", "kd_opt", "rtree_opt", "rstar_opt"]
    all_modes = ["brute", "bfs"] + tree_modes
    
    # We only want to test coverage variations
    combinations = [
        (False, False, True)
    ]
    
    # Load timestamps from dataset
    dataset_path = os.path.join("data", dataset)
    timestamps = []
    with open(dataset_path, "r") as f:
        reader = csv.reader(f)
        next(reader) # skip header
        for row in reader:
            if not row: continue
            timestamps.append(float(row[0]))
            
    n_rows = len(timestamps)

    # Generate ranges
    widths = [int(0.10 * n_rows)] * 3 + [int(0.40 * n_rows)] * 3 + [int(0.80 * n_rows)] * 3
    queries = []
    for w in widths:
        start_idx = random.randint(0, n_rows - w)
        end_idx = start_idx + w - 1
        
        start_ts = timestamps[start_idx]
        end_ts = timestamps[end_idx]
        
        if w == int(0.10 * n_rows): bucket = "small"
        elif w == int(0.40 * n_rows): bucket = "medium"
        else: bucket = "large"
            
        pos = "low" if start_idx < n_rows/3 else ("high" if start_idx > 2*n_rows/3 else "mid")
        
        queries.append({"start": start_ts, "end": end_ts, "bucket": bucket, "pos": pos, "width": w})
        
    csv_filename = "scratch/combinations_result_coverage_live_32k.csv"
    
    # Columns to match synthetic_result.csv
    headers = [
        "dataset", "n", "colors", "algorithm", "mode", "fairness_combination", 
        "epsilon", "delta", "weightA", "weightB", "alpha", "beta", 
        "num_diff_constraints", "num_ratio_constraints", "num_coverage_constraints", 
        "coverage_percent", "dimensions", "range_from", "range_to", "range_size", "range_bucket", 
        "range_position", "iterations_done", "file_load_ms", "build_time_ms", 
        "avg_query_time_ms", "min_query_time_ms", "max_query_time_ms", 
        "result", "result_from", "result_to", "similarity", "tle"
    ]
    
    with open(csv_filename, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=headers)
        writer.writeheader()
        
        for has_diff, has_ratio, has_cov in combinations:
            eps_to_test = epsilons if has_diff else [0.0]
            del_to_test = deltas if has_ratio else [0.0]
            # Vary coverage from 5% to 55% with increment of 10
            cov_to_test = list(range(5, 60, 10)) if has_cov else [0]
            
            for epsilon in eps_to_test:
                for delta in del_to_test:
                    for coverage_percent in cov_to_test:
                        num_diff = 2 if has_diff else 0
                        num_ratio = 1 if has_ratio else 0
                        num_cov = 1 if has_cov else 0
                
                        c_str = []
                        if has_diff: c_str.append("D")
                        if has_ratio: c_str.append("R")
                        if has_cov: c_str.append("C")
                        fair_combo = "+".join(c_str)
                
                        dims = num_diff + 2 * num_ratio
                
                        print(f"\nRunning Combination: {fair_combo} (cov: {coverage_percent}%)")
                
                        # Build constraints
                        input_lines = []
                        if has_diff:
                            input_lines.append("2")
                            input_lines.append(f"ETHUSDT SOLUSDT 1.0 1.0 {epsilon}")
                            input_lines.append(f"BTCUSDT SOLUSDT 1.0 1.0 {epsilon}")
                        else:
                            input_lines.append("0")
                            
                        if has_ratio:
                            input_lines.append("1")
                            input_lines.append(f"ETHUSDT BTCUSDT {weightA} {weightB} {delta}")
                        else:
                            input_lines.append("0")
                            
                        # Build query string
                        for q in queries:
                            q_str = f"{q['start']} {q['end']} {alpha} {beta}"
                            if has_cov:
                                req_points = int(q['width'] * (coverage_percent / 100.0))
                                q_str += f" {req_points} 0 0"
                            input_lines.append(q_str)
                            
                        # End queries
                        input_lines.append("-1")
                        input_lines.append("-1 -1 -1 -1")
                        input_data = "\n".join(input_lines) + "\n"
                
                        import sys
                        exe_ext = ".exe" if sys.platform == "win32" else ""
                
                        for mode in all_modes:
                            if mode == "brute":
                                exe = f"bin/main_brute_unified{exe_ext}"
                                arg_mode = "fair_cov" if has_cov else "fair"
                                alg_name = "brute"
                            elif mode == "bfs":
                                exe = f"bin/main_bfs_unified{exe_ext}"
                                arg_mode = "fair_cov" if has_cov else "fair"
                                alg_name = "bfs"
                            else:
                                exe = f"bin/main_coverage_unified{exe_ext}" if has_cov else f"bin/main_trees_unified{exe_ext}"
                                arg_mode = mode
                                alg_name = "trees"
                                
                            cmd = [exe, f"data/{dataset}", num_colors, arg_mode]
                            
                            print(f"--- Running {mode} ---")
                            try:
                                build_times = []
                                query_times = [[] for _ in range(len(queries))]
                                query_results = [{} for _ in range(len(queries))]
                                success_runs = 0
                                
                                for _ in range(3):
                                    result = subprocess.run(cmd, input=input_data, capture_output=True, text=True, timeout=60)
                                    
                                    if result.returncode != 0:
                                        print(f"Error running {mode}: {result.stderr}")
                                        continue
                                        
                                    success_runs += 1
                                    lines = result.stdout.split('\n')
                                    
                                    for line in lines:
                                        if "Built in" in line:
                                            m = re.search(r'Built in ([\d\.]+) ms', line)
                                            if m: build_times.append(float(m.group(1)))
                                    
                                    q_idx = 0
                                    curr_res = {}
                                    
                                    for line in lines:
                                        if "Best range" in line:
                                            m = re.search(r'Best range = \[(.*),(.*)\] \(similarity = ([\d\.]+)\)', line)
                                            if m:
                                                curr_res['result_from'] = m.group(1)
                                                curr_res['result_to'] = m.group(2)
                                                curr_res['result'] = f"[{m.group(1)},{m.group(2)}]"
                                                curr_res['similarity'] = m.group(3)
                                        
                                        if "Query executed in" in line:
                                            m = re.search(r'Query executed in ([\d\.]+) ms', line)
                                            if m:
                                                q_time = float(m.group(1))
                                                if q_idx < len(queries):
                                                    query_times[q_idx].append(q_time)
                                                    if not query_results[q_idx] and curr_res:
                                                        query_results[q_idx] = curr_res
                                                q_idx += 1
                                                curr_res = {}
                                                
                                if success_runs == 0:
                                    continue
                                    
                                avg_build = sum(build_times) / len(build_times) if build_times else 0
                                avg_build_str = f"{avg_build:.2f}" if build_times else "N/A"
                                
                                for q_idx, q in enumerate(queries):
                                    q_times = query_times[q_idx]
                                    q_res = query_results[q_idx]
                                    
                                    if q_times:
                                        min_q = min(q_times)
                                        max_q = max(q_times)
                                        avg_q = sum(q_times) / len(q_times)
                                        min_q_str = f"{min_q:.2f}"
                                        max_q_str = f"{max_q:.2f}"
                                        avg_q_str = f"{avg_q:.2f}"
                                    else:
                                        min_q_str = "N/A"
                                        max_q_str = "N/A"
                                        avg_q_str = "N/A"
                                        
                                    row = {
                                        "dataset": dataset,
                                        "n": n_rows,
                                        "colors": num_colors,
                                        "algorithm": alg_name,
                                        "mode": arg_mode,
                                        "fairness_combination": fair_combo,
                                        "epsilon": epsilon,
                                        "delta": delta,
                                        "weightA": weightA,
                                        "weightB": weightB,
                                        "alpha": alpha,
                                        "beta": beta,
                                        "num_diff_constraints": num_diff,
                                        "num_ratio_constraints": num_ratio,
                                        "num_coverage_constraints": num_cov,
                                        "coverage_percent": coverage_percent,
                                        "dimensions": dims,
                                        "range_from": q['start'],
                                        "range_to": q['end'],
                                        "range_size": q['end'] - q['start'] + 1,
                                        "range_bucket": q['bucket'],
                                        "range_position": q['pos'],
                                        "iterations_done": success_runs,
                                        "file_load_ms": "N/A",
                                        "build_time_ms": avg_build_str,
                                        "avg_query_time_ms": avg_q_str,
                                        "min_query_time_ms": min_q_str,
                                        "max_query_time_ms": max_q_str,
                                        "result": q_res.get('result', '[NA,NA]'),
                                        "result_from": q_res.get('result_from', 'NA'),
                                        "result_to": q_res.get('result_to', 'NA'),
                                        "similarity": q_res.get('similarity', '0.0'),
                                        "tle": "NO"
                                    }
                                    writer.writerow(row)
                                            
                            except subprocess.TimeoutExpired:
                                print(f"{mode} timed out! Recording TLE.")
                                for q in queries:
                                    row = {
                                        "dataset": dataset,
                                        "n": n_rows,
                                        "colors": num_colors,
                                        "algorithm": alg_name,
                                        "mode": arg_mode,
                                        "fairness_combination": fair_combo,
                                        "epsilon": epsilon,
                                        "delta": delta,
                                        "weightA": weightA,
                                        "weightB": weightB,
                                        "alpha": alpha,
                                        "beta": beta,
                                        "num_diff_constraints": num_diff,
                                        "num_ratio_constraints": num_ratio,
                                        "num_coverage_constraints": num_cov,
                                        "coverage_percent": coverage_percent,
                                        "dimensions": dims,
                                        "range_from": q['start'],
                                        "range_to": q['end'],
                                        "range_size": q['end'] - q['start'] + 1,
                                        "range_bucket": q['bucket'],
                                        "range_position": q['pos'],
                                        "iterations_done": 1,
                                        "file_load_ms": "N/A",
                                        "build_time_ms": "N/A",
                                        "avg_query_time_ms": "N/A",
                                        "min_query_time_ms": "N/A",
                                        "max_query_time_ms": "N/A",
                                        "result": "[NA,NA]",
                                        "result_from": "NA",
                                        "result_to": "NA",
                                        "similarity": "0.0",
                                        "tle": "YES"
                                    }
                                    writer.writerow(row)
                            except Exception as e:
                                print(f"Failed to execute {mode}: {e}")
                            
    print(f"Done. Results saved to {csv_filename}")

if __name__ == '__main__':
    main()
