import csv
import random
import re
import subprocess
import sys
from pathlib import Path


def main():
    # ============================================================
    # 1. CONFIGURATION
    # ============================================================

    dataset = "live_l2_2d_30k.txt"
    n_rows = 30000
    num_colors = "3"

    # We are benchmarking only dimensions 0 and 1 from the 5D dataset.
    dim_x = "0"
    dim_y = "1"

    alpha = 0.5
    beta = 0.5

    epsilons = [0.0, 2.0, 10.0, 100.0]
    deltas = [0.0, 0.05, 0.10, 0.20]

    weightA = 1.0
    weightB = 1.0

    repetitions = 3
    random.seed(42)

    # main_2d supports these modes.
    # Brute force is intentionally disabled for the full 30K dataset.
    modes = ["bfs", "kd_opt"]
    # modes = ["brute", "bfs", "kd_opt"]  # Use only on a small dataset.

    # 2D implementation currently supports:
    # D   = Difference only
    # R   = Ratio only
    # D+R = Difference + Ratio
    combinations = [
        (True, False),
        (False, True),
        (True, True),
    ]

    # All three color pairs are tested.
    color_pairs = [
        ("1", "2"),
        ("1", "3"),
        ("2", "3"),
    ]

    project_root = Path(__file__).resolve().parent.parent
    dataset_path = project_root / "data" / dataset
    exe_ext = ".exe" if sys.platform == "win32" else ""
    executable = project_root / "bin" / f"main_2d{exe_ext}"
    csv_filename = project_root / "scratch" / "combinations_result_2d.csv"

    # ============================================================
    # 2. CHECK FILES
    # ============================================================

    if not dataset_path.exists():
        print(f"Dataset not found: {dataset_path}")
        return

    if not executable.exists():
        print(f"Executable not found: {executable}")
        print("Run: make two_d")
        return

    # ============================================================
    # 3. READ THE SELECTED 2D COORDINATES
    # ============================================================

    points = read_points(
        dataset_path=dataset_path,
        n_rows=n_rows,
        dim_x=int(dim_x),
        dim_y=int(dim_y),
    )

    if len(points) != n_rows:
        print(f"Warning: expected {n_rows} points, loaded {len(points)}")
        n_rows = len(points)

    print(f"Dataset: {dataset}")
    print(f"Points: {n_rows}")
    print(f"2D projection: dim {dim_x} and dim {dim_y}")

    # ============================================================
    # 4. GENERATE SMALL / MEDIUM / LARGE 2D QUERY RECTANGLES
    #
    # This mirrors the 1D script structurally:
    #   3 small  queries: 10%
    #   3 medium queries: 40%
    #   3 large  queries: 80%
    #
    # For 2D, the percentage is applied independently to the
    # sorted X and sorted Y coordinate ranges.
    # ============================================================

    queries = generate_queries(points)

    print("\nGenerated query rectangles:")
    for q in queries:
        print(
            f"{q['query_id']:8s} "
            f"X=[{q['qx_min']}, {q['qx_max']}] "
            f"Y=[{q['qy_min']}, {q['qy_max']}] "
            f"bucket={q['bucket']} position={q['pos']}"
        )

    # ============================================================
    # 5. CSV OUTPUT COLUMNS
    # ============================================================

    headers = [
        "dataset",
        "n",
        "colors",
        "algorithm",
        "mode",
        "fairness_combination",
        "epsilon",
        "delta",
        "weightA",
        "weightB",
        "alpha",
        "beta",
        "num_diff_constraints",
        "num_ratio_constraints",
        "dimensions",
        "dim_x",
        "dim_y",
        "qx_min",
        "qx_max",
        "qy_min",
        "qy_max",
        "query_id",
        "range_bucket",
        "range_position",
        "iterations_done",
        "file_load_ms",
        "build_time_ms",
        "avg_query_time_ms",
        "min_query_time_ms",
        "max_query_time_ms",
        "result",
        "result_x_min",
        "result_x_max",
        "result_y_min",
        "result_y_max",
        "similarity",
        "tle",
    ]

    csv_filename.parent.mkdir(parents=True, exist_ok=True)

    # ============================================================
    # 6. RUN ALL FAIRNESS COMBINATIONS
    # ============================================================

    with open(csv_filename, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=headers)
        writer.writeheader()

        for has_diff, has_ratio in combinations:
            eps_to_test = epsilons if has_diff else [0.0]
            del_to_test = deltas if has_ratio else [0.0]

            num_diff = len(color_pairs) if has_diff else 0
            num_ratio = len(color_pairs) if has_ratio else 0

            combo_parts = []
            if has_diff:
                combo_parts.append("D")
            if has_ratio:
                combo_parts.append("R")
            fair_combo = "+".join(combo_parts)

            # Same fairness-space dimensionality rule as the 1D code:
            # each Difference constraint contributes 1 dimension;
            # each Ratio constraint contributes 2 dimensions.
            fairness_dimensions = num_diff + 2 * num_ratio

            for epsilon in eps_to_test:
                for delta in del_to_test:
                    print("\n==================================================")
                    print(f"Combination : {fair_combo}")
                    print(f"Epsilon     : {epsilon}")
                    print(f"Delta       : {delta}")
                    print("==================================================")

                    input_data = build_input(
                        queries=queries,
                        has_diff=has_diff,
                        has_ratio=has_ratio,
                        epsilon=epsilon,
                        delta=delta,
                        color_pairs=color_pairs,
                        weightA=weightA,
                        weightB=weightB,
                        alpha=alpha,
                        beta=beta,
                    )

                    for mode in modes:
                        print(f"\n--- Running {mode} ---")

                        cmd = [
                            str(executable),
                            str(dataset_path),
                            num_colors,
                            dim_x,
                            dim_y,
                            mode,
                        ]

                        build_times = []
                        query_times = [[] for _ in queries]
                        query_results = [{} for _ in queries]
                        success_runs = 0

                        for run_id in range(repetitions):
                            print(f"Run {run_id + 1}/{repetitions}")

                            try:
                                result = subprocess.run(
                                    cmd,
                                    input=input_data,
                                    capture_output=True,
                                    text=True,
                                    timeout=3600,
                                )
                            except subprocess.TimeoutExpired:
                                print(f"TIMEOUT: {mode}")
                                continue

                            if result.returncode != 0:
                                print(f"Error running {mode}:")
                                print(result.stderr)
                                continue

                            success_runs += 1

                            build_time, times, results = parse_output(
                                result.stdout,
                                len(queries),
                            )

                            if build_time is not None:
                                build_times.append(build_time)

                            for q_idx in range(len(queries)):
                                if times[q_idx] is not None:
                                    query_times[q_idx].append(times[q_idx])

                                if not query_results[q_idx] and results[q_idx]:
                                    query_results[q_idx] = results[q_idx]

                        if success_runs == 0:
                            print(f"No successful runs for {mode}")
                            continue

                        avg_build = (
                            sum(build_times) / len(build_times)
                            if build_times
                            else None
                        )

                        # ====================================================
                        # 7. WRITE ONE CSV ROW PER QUERY RECTANGLE
                        # ====================================================

                        for q_idx, q in enumerate(queries):
                            times = query_times[q_idx]
                            q_res = query_results[q_idx]

                            if times:
                                min_q = min(times)
                                max_q = max(times)
                                avg_q = sum(times) / len(times)

                                min_q_str = f"{min_q:.4f}"
                                max_q_str = f"{max_q:.4f}"
                                avg_q_str = f"{avg_q:.4f}"
                            else:
                                min_q_str = "N/A"
                                max_q_str = "N/A"
                                avg_q_str = "N/A"

                            avg_build_str = (
                                f"{avg_build:.4f}"
                                if avg_build is not None
                                else "N/A"
                            )

                            row = {
                                "dataset": dataset,
                                "n": n_rows,
                                "colors": num_colors,
                                "algorithm": mode,
                                "mode": mode,
                                "fairness_combination": fair_combo,
                                "epsilon": epsilon,
                                "delta": delta,
                                "weightA": weightA,
                                "weightB": weightB,
                                "alpha": alpha,
                                "beta": beta,
                                "num_diff_constraints": num_diff,
                                "num_ratio_constraints": num_ratio,
                                "dimensions": fairness_dimensions,
                                "dim_x": dim_x,
                                "dim_y": dim_y,
                                "qx_min": q["qx_min"],
                                "qx_max": q["qx_max"],
                                "qy_min": q["qy_min"],
                                "qy_max": q["qy_max"],
                                "query_id": q["query_id"],
                                "range_bucket": q["bucket"],
                                "range_position": q["pos"],
                                "iterations_done": success_runs,
                                "file_load_ms": "N/A",
                                "build_time_ms": avg_build_str,
                                "avg_query_time_ms": avg_q_str,
                                "min_query_time_ms": min_q_str,
                                "max_query_time_ms": max_q_str,
                                "result": q_res.get(
                                    "result",
                                    "[NA,NA] x [NA,NA]",
                                ),
                                "result_x_min": q_res.get(
                                    "result_x_min",
                                    "NA",
                                ),
                                "result_x_max": q_res.get(
                                    "result_x_max",
                                    "NA",
                                ),
                                "result_y_min": q_res.get(
                                    "result_y_min",
                                    "NA",
                                ),
                                "result_y_max": q_res.get(
                                    "result_y_max",
                                    "NA",
                                ),
                                "similarity": q_res.get(
                                    "similarity",
                                    "0.0",
                                ),
                                "tle": "NO",
                            }

                            writer.writerow(row)
                            f.flush()

                        print(f"Finished {mode}")

    print(f"\nDone. Results saved to {csv_filename}")


# ============================================================
# HELPER: READ POINTS
# ============================================================

def read_points(dataset_path, n_rows, dim_x, dim_y):
    """
    Reads only the coordinate section of the original dataset:

        30000
        5 Dims
        d0 d1 d2 d3 d4
        ...
        colors...

    The colors at the end are not needed by Python because main_2d
    reads them itself. Python only needs X and Y values to create
    benchmark query rectangles.
    """

    points = []

    with open(dataset_path, "r") as f:
        declared_n = int(f.readline().strip())

        dim_line = f.readline().strip()
        match = re.search(r"(\d+)", dim_line)

        if not match:
            raise ValueError(f"Invalid dimension line: {dim_line}")

        num_dims = int(match.group(1))

        if dim_x >= num_dims or dim_y >= num_dims:
            raise ValueError(
                f"Requested dims ({dim_x}, {dim_y}), "
                f"but dataset has only {num_dims} dimensions"
            )

        rows_to_read = min(n_rows, declared_n)

        for i in range(rows_to_read):
            line = f.readline()

            if not line:
                raise ValueError(
                    f"Dataset ended while reading point {i}"
                )

            values = list(map(float, line.split()))

            points.append(
                (values[dim_x], values[dim_y])
            )

    return points


# ============================================================
# HELPER: GENERATE 2D QUERIES
# ============================================================

def generate_queries(points):
    """
    Creates exactly 9 queries, matching the structure of the 1D script:

        3 small  queries -> 10% coordinate windows
        3 medium queries -> 40% coordinate windows
        3 large  queries -> 80% coordinate windows

    Position is classified from the rectangle center in normalized
    X/Y rank space:
        low  -> center lies in the lower third
        high -> center lies in the upper third
        mid  -> otherwise
    """

    xs = sorted(p[0] for p in points)
    ys = sorted(p[1] for p in points)

    n = len(points)

    bucket_specs = [
        ("small", 0.10, 3),
        ("medium", 0.40, 3),
        ("large", 0.80, 3),
    ]

    queries = []

    for bucket, fraction, count in bucket_specs:
        window = max(2, int(fraction * n))

        for query_number in range(1, count + 1):
            x_start = random.randint(0, n - window)
            y_start = random.randint(0, n - window)

            x_end = x_start + window - 1
            y_end = y_start + window - 1

            x_mid_rank = (x_start + x_end) / 2
            y_mid_rank = (y_start + y_end) / 2
            combined_mid_rank = (x_mid_rank + y_mid_rank) / 2

            if combined_mid_rank < n / 3:
                pos = "low"
            elif combined_mid_rank > 2 * n / 3:
                pos = "high"
            else:
                pos = "mid"

            queries.append({
                "query_id": f"{bucket}_{query_number}",
                "qx_min": xs[x_start],
                "qx_max": xs[x_end],
                "qy_min": ys[y_start],
                "qy_max": ys[y_end],
                "bucket": bucket,
                "pos": pos,
            })

    return queries


# ============================================================
# HELPER: BUILD CONSOLE INPUT FOR main_2d
# ============================================================

def build_input(
    queries,
    has_diff,
    has_ratio,
    epsilon,
    delta,
    color_pairs,
    weightA,
    weightB,
    alpha,
    beta,
):
    input_lines = []

    # Difference constraints
    if has_diff:
        input_lines.append(str(len(color_pairs)))

        for colorA, colorB in color_pairs:
            input_lines.append(
                f"{colorA} {colorB} "
                f"{weightA} {weightB} {epsilon}"
            )
    else:
        input_lines.append("0")

    # Ratio constraints
    if has_ratio:
        input_lines.append(str(len(color_pairs)))

        for colorA, colorB in color_pairs:
            input_lines.append(
                f"{colorA} {colorB} "
                f"{weightA} {weightB} {delta}"
            )
    else:
        input_lines.append("0")

    # Query rectangles
    for q in queries:
        input_lines.append(
            f"{q['qx_min']} {q['qx_max']} "
            f"{q['qy_min']} {q['qy_max']} "
            f"{alpha} {beta}"
        )

    # Exit inner query loop
    input_lines.append("-1")

    # Exit outer constraint loop
    input_lines.append("-1")

    return "\n".join(input_lines) + "\n"


# ============================================================
# HELPER: PARSE main_2d OUTPUT
# ============================================================

def parse_output(output, num_queries):
    build_time = None
    query_times = []
    query_results = []

    # Example:
    # 2D kd_opt mode. Loaded in 25.1234 ms
    build_match = re.search(
        r"Loaded in\s+([\d.]+)\s+ms",
        output,
    )

    if build_match:
        build_time = float(build_match.group(1))

    rectangle_pattern = re.compile(
        r"Best rectangle\s*=\s*"
        r"\[([^,\]]+),([^\]]+)\]\s*x\s*"
        r"\[([^,\]]+),([^\]]+)\]\s*"
        r"\(similarity\s*=\s*([^)]+)\)"
    )

    for match in rectangle_pattern.finditer(output):
        x_min = match.group(1).strip()
        x_max = match.group(2).strip()
        y_min = match.group(3).strip()
        y_max = match.group(4).strip()
        similarity = match.group(5).strip()

        query_results.append({
            "result": f"[{x_min},{x_max}] x [{y_min},{y_max}]",
            "result_x_min": x_min,
            "result_x_max": x_max,
            "result_y_min": y_min,
            "result_y_max": y_max,
            "similarity": similarity,
        })

    time_pattern = re.compile(
        r"Query executed in\s+([\d.]+)\s+ms"
    )

    for match in time_pattern.finditer(output):
        query_times.append(float(match.group(1)))

    while len(query_times) < num_queries:
        query_times.append(None)

    while len(query_results) < num_queries:
        query_results.append({})

    return (
        build_time,
        query_times[:num_queries],
        query_results[:num_queries],
    )


if __name__ == "__main__":
    main()