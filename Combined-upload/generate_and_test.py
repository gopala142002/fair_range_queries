import os
import subprocess
import time
import sys

def main():
    test_file = "data/live_l2_5D.txt"
    exe_ext = ".exe" if sys.platform == "win32" else ""
    
    queries = [
        {
            "name": "Test 1",
            "qx_min": "1779111784.092920", "qx_max": "1779113385.710762",
            "qy_min": "84.670000", "qy_max": "76936.500000",
            "diffs": [(1, 2, 2568), (1, 3, 1359), (2, 3, 1209)],
            "ratios": [(1, 2, 1.3069), (1, 3, 0.4282), (2, 3, 0.3809)]
        },
        {
            "name": "Test 2",
            "qx_min": "1779111650.859551", "qx_max": "1779113634.435026",
            "qy_min": "84.590000", "qy_max": "76941.000000",
            "diffs": [(1, 2, 2902), (1, 3, 1259), (2, 3, 1643)],
            "ratios": [(1, 2, 1.0865), (1, 3, 0.2918), (2, 3, 0.3809)]
        },
        {
            "name": "Test 3",
            "qx_min": "1779111091.598917", "qx_max": "1779114567.355902",
            "qy_min": "84.490000", "qy_max": "77003.900000",
            "diffs": [(1, 2, 4402), (1, 3, 1305), (2, 3, 3097)],
            "ratios": [(1, 2, 0.8273), (1, 3, 0.1550), (2, 3, 0.3679)]
        },
        {
            "name": "Test 4",
            "qx_min": "1779111079.902913", "qx_max": "1779114690.685747",
            "qy_min": "84.480000", "qy_max": "77011.403992",
            "diffs": [(1, 2, 4546), (1, 3, 1236), (2, 3, 3310)],
            "ratios": [(1, 2, 0.8184), (1, 3, 0.1394), (2, 3, 0.3734)]
        }
    ]

    for q in queries:
        print(f"\n==================== {q['name']} ====================")
        query_args = [q["qx_min"], q["qx_max"], q["qy_min"], q["qy_max"]]
        print(f"Query Bounds: X:[{q['qx_min']}, {q['qx_max']}] Y:[{q['qy_min']}, {q['qy_max']}]")
        
        # Build constraint input
        delta = 0.05  # 5% tolerance for ratios
        input_lines = []
        input_lines.append(str(len(q["diffs"])))
        for d in q["diffs"]:
            # Format: ColorA ColorB weightA weightB epsilon
            input_lines.append(f"{d[0]} {d[1]} 1.0 1.0 {d[2]}")
        input_lines.append(str(len(q["ratios"])))
        for r in q["ratios"]:
            # Format: ColorA ColorB weightA weightB delta
            # User specified: weights are 1.0, and the value is the delta
            input_lines.append(f"{r[0]} {r[1]} 1.0 1.0 {r[2]}")
        input_lines.append("")
        constraint_input = "\n".join(input_lines)
        
        # Define executables to run
        exes = [
            ("OLD KD-TREE 2D (Unoptimized)", f"main_kd2d_old{exe_ext}"),
            ("NEW KD-TREE 2D (Optimized X->Y)", f"main_kd2d{exe_ext}"),
            ("NEW KD-TREE 2D YX (Optimized Y->X)", f"main_kd2d_yx{exe_ext}")
        ]
        
        for title, exe_name in exes:
            print(f"\n--- Running {title} ---")
            exe_path = os.path.join(".", "bin", exe_name)
            start_time = time.time()
            try:
                result = subprocess.run(
                    [exe_path, test_file] + query_args,
                    input=constraint_input,
                    capture_output=True,
                    text=True,
                    check=True
                )
                elapsed = time.time() - start_time
                print(f"Time: {elapsed:.4f}s")
                # Filter out the "Enter number of..." prompts for cleaner output
                output_lines = result.stdout.strip().split('\n')
                clean_out = []
                for line in output_lines:
                    if "Enter number of" not in line and "Constraint " not in line and "Loaded " not in line:
                        clean_out.append(line)
                print("Output:")
                print("\n".join(clean_out))
            except subprocess.CalledProcessError as e:
                print(f"Error running {title}: {e}")
                print(e.stderr)

if __name__ == "__main__":
    main()
