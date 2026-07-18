import os
import random
import re
import subprocess
import sys
import time
import numpy as np

def load_data(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()
        
    n = int(lines[0].strip())
    d_t = lines[1].strip()
    d = int(d_t.split()[0])
    
    coords_lines = lines[2:2+n]
    colors_line = lines[2+n].strip()
    colors = list(map(int, colors_line.split()))
    
    coords = []
    for line in coords_lines:
        coords.append(list(map(float, line.strip().split())))
        
    return n, d, np.array(coords), np.array(colors)

def generate_queries(coords, n, d, colors, num_per_bucket=1):
    buckets = {
        'b1': {'min_pts': int(0.20 * n), 'max_pts': int(0.40 * n), 'queries': []},
        'b2': {'min_pts': int(0.40 * n), 'max_pts': int(0.60 * n), 'queries': []},
        'b3': {'min_pts': int(0.60 * n), 'max_pts': int(0.80 * n), 'queries': []},
        'b4': {'min_pts': int(0.80 * n), 'max_pts': int(1.00 * n), 'queries': []},
    }
    
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
                vals = coords[:, k]
                width = min(100.0, max(1.0, random.gauss(base_width, 10.0)))
                p_low = random.uniform(0.0, 100.0 - width)
                p_high = p_low + width
                
                min_v = float(np.percentile(vals, p_low))
                max_v = float(np.percentile(vals, p_high))
                qbox.append((min_v, max_v))
                
            mask = np.ones(n, dtype=bool)
            for k in range(d):
                mask &= (coords[:, k] >= qbox[k][0]) & (coords[:, k] <= qbox[k][1])
            
            in_box = np.sum(mask)
            
            if b_info['min_pts'] <= in_box <= b_info['max_pts']:
                box_colors = colors[mask]
                unique, ucounts = np.unique(box_colors, return_counts=True)
                counts = {c: 0 for c in range(1, 6)}
                for c_val, cnt in zip(unique, ucounts):
                    if c_val in counts:
                        counts[c_val] = cnt
                
                if any(cnt < 50 for cnt in counts.values()):
                    continue
                    
                tight_qbox = []
                for k in range(d):
                    tight_min = np.min(coords[mask, k])
                    tight_max = np.max(coords[mask, k])
                    tight_qbox.append((tight_min, tight_max))
                    
                b_info['queries'].append((tight_qbox, in_box, mask, counts))
                print(f"  -> Found query for {b_name} with {in_box} points at attempt {attempts}")
                
    return buckets

def get_constraint_string(diff_matrix, ratio_matrix):
    d_lines = []
    for r in range(1, 6):
        row = []
        for c in range(1, 6):
            if r == c:
                row.append("NaN")
            else:
                val = diff_matrix.get((r, c), "NaN")
                row.append(str(val))
        d_lines.append(" ".join(row))
        
    diff_block = "\n".join(d_lines)
    
    r_lines = []
    for r in range(1, 6):
        row = []
        for c in range(1, 6):
            if r == c:
                row.append("NaN")
            else:
                val = ratio_matrix.get((r, c), "NaN")
                row.append(str(val))
        r_lines.append(" ".join(row))
        
    ratio_block = "\n".join(r_lines)
    
    return f"""Weights of color:
1.0 1.0 1.0 1.0 1.0

Coverage:
50 50 50 50 50

Difference:
{diff_block}

Ratio:
{ratio_block}
"""

def write_dataset_file(filename, n, d, coords, colors, qbox=None):
    with open(filename, 'w') as f:
        f.write(f"{n}\n")
        f.write(f"{d} 0\n")
        
        for p in coords:
            row_str = " ".join(str(val) for val in p)
            f.write(f"{row_str}\n")
            
        colors_str = " ".join(str(c) for c in colors)
        f.write(f"{colors_str}\n")
        
        if qbox is not None:
            for min_v, max_v in qbox:
                f.write(f"{min_v} {max_v}\n")

def run_experiment():
    dataset_file = "NYC_30k_5colors.txt"
    temp_data_file = "temp_anytime_data.txt"
    temp_constraints = "temp_anytime_constraints.txt"
    log_file = "anytime_study_log_nyc.txt"
    
    with open(log_file, "w") as f:
        f.write("="*100 + "\n")
        f.write("ANYTIME STUDY EXPERIMENT\n")
        f.write("="*100 + "\n\n")
        
    print(f"Loading {dataset_file}...")
    n, d, coords, colors = load_data(dataset_file)
    
    buckets = generate_queries(coords, n, d, colors, num_per_bucket=1)
    
    solvers = [
        {"name": "Dinkelbach_WS_Grid", "cmd": ["timeout", "1800", "python3", "-u", "solve_ilp_anytime_dynamic_grid_dinkelbach_ws.py"]},
        {"name": "Dinkelbach_NoWS", "cmd": ["timeout", "1800", "python3", "-u", "solve_ilp_anytime_dinkelbach_nows.py"]},
        {"name": "BinarySearch_NoGrid", "cmd": ["timeout", "1800", "python3", "-u", "solve_ilp_anytime_binary_nogrid.py"]},
        {"name": "BinarySearch_WS_Grid", "cmd": ["timeout", "1800", "python3", "-u", "solve_ilp_anytime_dynamic_grid_binary_ws.py"]}
    ]
    
    for b_name, b_info in buckets.items():
        if not b_info['queries']:
            print(f"Skipping {b_name} (no valid queries found)")
            with open(log_file, "a") as f:
                f.write(f"Skipping {b_name} (no valid queries found)\n")
            continue
            
        qbox, in_box, mask, counts = b_info['queries'][0]
        
        print(f"\n{'='*50}")
        print(f"RUNNING EXPERIMENT FOR {b_name} ({in_box} points)")
        print(f"{'='*50}")
        
        diff_12 = abs(counts[1] - counts[2])
        real_ratio = counts[1] / counts[3] if counts[3] > 0 else 1.0
        ratio_13 = real_ratio - 1.0 if real_ratio > 1.0 else (counts[3] / counts[1]) - 1.0
        
        diff_constraint = max(1, diff_12 - 50)
        ratio_constraint = max(0.01, ratio_13 - 0.01)
        
        print(f"Box Color Counts: {counts}")
        print(f"Existing diff_12: {diff_12} -> Constraint: {diff_constraint}")
        print(f"Existing ratio_13: {ratio_13:.4f} -> Constraint: {ratio_constraint:.4f}")
        
        with open(log_file, "a") as f:
            f.write(f"\n{'='*50}\n")
            f.write(f"RUNNING EXPERIMENT FOR {b_name} ({in_box} points)\n")
            f.write(f"{'='*50}\n")
            f.write(f"Box Color Counts: {counts}\n")
            f.write(f"Existing diff_12: {diff_12} -> Constraint: {diff_constraint}\n")
            f.write(f"Existing ratio_13: {ratio_13:.4f} -> Constraint: {ratio_constraint:.4f}\n")
        
        diff_matrix = {(1, 2): diff_constraint, (2, 1): diff_constraint}
        ratio_matrix = {(1, 3): ratio_constraint, (3, 1): ratio_constraint}
        
        constraint_str = get_constraint_string(diff_matrix, ratio_matrix)
        with open(temp_constraints, 'w') as f:
            f.write(constraint_str)
            
        write_dataset_file(temp_data_file, n, d, coords, colors, qbox)
        
        for solver in solvers:
            print(f"\n>>> Running {solver['name']} ...")
            with open(log_file, "a") as f:
                f.write(f"\n>>> Running {solver['name']} ...\n")
                
            cmd = solver['cmd'] + [temp_data_file, temp_constraints]
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            
            final_sim = "N/A"
            final_time = "N/A"
            
            while True:
                line = proc.stdout.readline()
                if not line:
                    break
                line = line.strip()
                
                print(f"  [{solver['name']}] {line}")
                with open(log_file, "a") as f:
                    f.write(f"  [{solver['name']}] {line}\n")
                
                if "Jaccard Similarity:" in line:
                    final_sim = line.split(":")[-1].strip()
                elif "Time taken" in line:
                    final_time = line.split(":")[1].replace("seconds", "").strip()
                    
            ret_code = proc.wait()
            if ret_code == 124:
                print(f"  [{solver['name']}] TIMEOUT REACHED (1800s).")
                with open(log_file, "a") as f:
                    f.write(f"  [{solver['name']}] TIMEOUT REACHED (1800s).\n")
                final_sim = "TIMEOUT"
                final_time = ">1800"
            
            print(f"  [{solver['name']}] FINAL SIM: {final_sim} | FINAL TIME: {final_time}s")
            with open(log_file, "a") as f:
                f.write(f"  [{solver['name']}] FINAL SIM: {final_sim} | FINAL TIME: {final_time}s\n")

if __name__ == '__main__':
    run_experiment()
