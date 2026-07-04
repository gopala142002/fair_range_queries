import csv
import re
import subprocess
import sys
from pathlib import Path


def main():
    dataset = "live_l2_2d_30k.txt"
    n_rows = 30000
    num_colors = "3"

    dim_x = "0"
    dim_y = "1"

    alpha = 0.5
    beta = 0.5

    epsilons = [0.0,2.0,10.0,100.0]

    deltas = [0.0,0.05,0.10,0.20,]

    weightA = 1.0
    weightB = 1.0

    coverage_requirements = [0,100,0]

    repetitions = 3

    modes = ["bfs","kd_opt",]

    combinations = [
        (True, False, False),    
        (False, True, False),    
        (False, False, True),    
        (True, True, False),     
        (True, False, True),     
        (False, True, True),    
        (True, True, True),     
    ]

    color_pairs = [("1", "2"),("1", "3"),("2", "3"),]

    project_root = Path(__file__).resolve().parent.parent

    dataset_path = (
        project_root
        / "data"
        / dataset
    )

    exe_ext = (
        ".exe"
        if sys.platform == "win32"
        else ""
    )

    executable = (
        project_root
        / "bin"
        / f"main_2d{exe_ext}"
    )

    csv_filename = (
        project_root
        / "scratch"
        / "combinations_result_2d.csv"
    )

    if not dataset_path.exists():
        print(
            f"Dataset not found: "
            f"{dataset_path}"
        )
        return


    if not executable.exists():
        print(
            f"Executable not found: "
            f"{executable}"
        )
        print(
            "Run: make two_d"
        )
        return


    points = read_points(
        dataset_path=dataset_path,
        n_rows=n_rows,
        dim_x=int(dim_x),
        dim_y=int(dim_y),
    )


    if len(points) != n_rows:
        n_rows = len(points)


    queries = generate_queries(
        points
    )


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
        "num_coverage_constraints",
        "dimensions",
        "dim_x",
        "dim_y",
        "qx_min",
        "qx_max",
        "qy_min",
        "qy_max",
        "query_id",
        "query_point_count",
        "query_selectivity_percent",
        "range_bucket",
        "range_position",
        "iterations_done",
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


    csv_filename.parent.mkdir(
        parents=True,
        exist_ok=True
    )


    with open(csv_filename,"w",newline="",) as f:
        writer = csv.DictWriter(
            f,
            fieldnames=headers,
        )
        writer.writeheader()
        for(has_diff,has_ratio,has_cov,) in combinations:
            if has_diff:
                eps_to_test = epsilons
            else:
                eps_to_test = [0.0]

            if has_ratio:
                del_to_test = deltas
            else:
                del_to_test = [0.0]

            num_diff = (len(color_pairs) if has_diff else 0)

            num_ratio = (len(color_pairs) if has_ratio else 0)

            num_cov = (len(coverage_requirements) if has_cov else 0)

            combo_parts = []
            if has_diff:
                combo_parts.append("D")
            if has_ratio:
                combo_parts.append("R")
            if has_cov:
                combo_parts.append("C")

            fair_combo = "+".join(combo_parts)
            fairness_dimensions = (num_diff+ 2 * num_ratio)


            for epsilon in eps_to_test:
                for delta in del_to_test:
                    print(
                        f"\nRunning Combination: "
                        f"{fair_combo}"
                    )
                    input_data = build_input(
                        queries=queries,
                        has_diff=has_diff,
                        has_ratio=has_ratio,
                        has_cov=has_cov,
                        epsilon=epsilon,
                        delta=delta,
                        color_pairs=color_pairs,
                        weightA=weightA,
                        weightB=weightB,
                        alpha=alpha,
                        beta=beta,
                        coverage_requirements=coverage_requirements,
                    )

                    for mode in modes:
                        if has_cov:
                            if mode == "bfs":
                                run_mode = ("bfs_cov")
                            else:
                                run_mode = ("kd_opt_cov")
                        else:
                            run_mode = mode

                        print(f"--- Running "f"{run_mode} ---")
                        cmd = [str(executable),str(dataset_path),num_colors,dim_x,dim_y,run_mode,]
                        build_times = []
                        query_times = [[] for _ in queries]

                        query_results = [{} for _ in queries]

                        success_runs = 0
                        timeout_runs = 0

                        for _ in range(repetitions):
                            try:
                                result = (
                                    subprocess.run(
                                        cmd,
                                        input=input_data,
                                        capture_output=True,
                                        text=True,
                                        timeout=300,
                                    )
                                )
                            except subprocess.TimeoutExpired:
                                timeout_runs += 1
                                print(f"TIMEOUT: {run_mode}")
                                continue
                            if result.returncode != 0:
                                print(f"Error running "f"{run_mode}:")
                                print(result.stderr)
                                continue

                            success_runs += 1
                            (build_time,times,results,) = parse_output(result.stdout,len(queries),)

                            if build_time is not None:
                                build_times.append(build_time)


                            for q_idx in range(len(queries)):
                                if (times[q_idx] is not None):
                                    query_times[q_idx].append(times[q_idx])

                                if (not query_results[q_idx] and results[q_idx]):
                                    query_results[q_idx] = results[q_idx]

                        if build_times:
                            avg_build = (sum(build_times)/ len(build_times))
                        else:
                            avg_build = None

                        for q_idx, q in enumerate(queries):
                            times = query_times[q_idx]
                            q_res = query_results[q_idx]

                            if times:
                                min_q = min(times)
                                max_q = max(times)
                                avg_q = (sum(times)/ len(times))

                                min_q_str = (f"{min_q:.4f}")
                                max_q_str = (f"{max_q:.4f}")
                                avg_q_str = (f"{avg_q:.4f}")

                            else:
                                min_q_str = "N/A"
                                max_q_str = "N/A"
                                avg_q_str = "N/A"

                            if avg_build is not None:
                                avg_build_str = (f"{avg_build:.4f}")

                            else:
                                avg_build_str = ("N/A")

                            row = {
                                "dataset":dataset,
                                "n":n_rows,
                                "colors":num_colors,
                                "algorithm":mode,
                                "mode":run_mode,
                                "fairness_combination":fair_combo,
                                "epsilon":epsilon,
                                "delta":delta,
                                "weightA":weightA,
                                "weightB":weightB,
                                "alpha":alpha,
                                "beta":beta,
                                "num_diff_constraints":num_diff,
                                "num_ratio_constraints":num_ratio,
                                "num_coverage_constraints":num_cov,
                                "dimensions":fairness_dimensions,
                                "dim_x":dim_x,
                                "dim_y":dim_y,
                                "qx_min":q["qx_min"],
                                "qx_max":q["qx_max"],
                                "qy_min":q["qy_min"],
                                "qy_max":q["qy_max"],
                                "query_id":q["query_id"],
                                "query_point_count":q["point_count"],
                                "query_selectivity_percent":f"{q['selectivity']:.4f}",
                                "range_bucket":q["bucket"],
                                "range_position":q["pos"],
                                "iterations_done":success_runs,
                                "build_time_ms":avg_build_str,
                                "avg_query_time_ms":avg_q_str,
                                "min_query_time_ms":min_q_str,
                                "max_query_time_ms":max_q_str,
                                "result":q_res.get("result","[NA,NA] x [NA,NA]",),
                                "result_x_min":q_res.get("result_x_min","NA",),
                                "result_x_max":q_res.get("result_x_max","NA"),
                                "result_y_min":q_res.get("result_y_min","NA",),
                                "result_y_max":q_res.get("result_y_max","NA",),
                                "similarity":q_res.get("similarity","0.0",),
                                "tle":"YES" if timeout_runs == repetitions else "NO",
                            }

                            writer.writerow(row)
                            f.flush()


    print(f"\nDone. Results saved to "f"{csv_filename}")


def read_points(dataset_path,n_rows,dim_x,dim_y,):
    points = []
    with open(dataset_path,"r",) as f:
        declared_n = int(f.readline().strip())

        dim_line = (f.readline().strip())

        match = re.search(r"(\d+)",dim_line,)

        if not match:
            raise ValueError(f"Invalid dimension line: "f"{dim_line}")

        num_dims = int(match.group(1))

        if (dim_x >= num_dims or dim_y >= num_dims):
            raise ValueError(

                f"Requested dims "
                f"({dim_x}, {dim_y}), "
                f"but dataset has "
                f"{num_dims} dimensions"
            )

        rows_to_read = min(n_rows,declared_n)


        for i in range(rows_to_read):
            line = f.readline()
            if not line:
                raise ValueError(f"Dataset ended while "f"reading point {i}")
            values = list(map(float,line.split()))

            points.append((values[dim_x],values[dim_y]))
    return points




def generate_queries(points):
    xs = sorted(p[0] for p in points)
    ys = sorted(p[1] for p in points)
    n = len(points)
    bucket_specs = [("small", 0.10),("medium", 0.40),("large", 0.80),]

    positions = [("low", 0.0),("mid", 0.5),("high", 1.0),]

    queries = []

    for (bucket,fraction,) in bucket_specs:
        window = max(2,int(fraction * n))
        max_start = (n - window)
        for(pos,relative_start) in positions:
            start_idx = int(
                round(relative_start* max_start
                )
            )
            end_idx = (start_idx+ window- 1)
            qx_min = xs[start_idx]
            qx_max = xs[end_idx]
            qy_min = ys[start_idx]
            qy_max = ys[end_idx]
            point_count = sum(1 for x, y in points
                if (qx_min<= x<= qx_max and qy_min <= y <= qy_max))
            selectivity = ( 100.0 * point_count  / n)

               

            queries.append({
                "query_id":f"{bucket}_{pos}",
                "qx_min":qx_min,
                "qx_max":qx_max,
                "qy_min":qy_min,
                "qy_max":qy_max,
                "bucket":bucket,
                "pos":pos,
                "point_count":point_count,
                "selectivity":selectivity,
            })
    return queries




def build_input(
    queries,
    has_diff,
    has_ratio,
    has_cov,
    epsilon,
    delta,
    color_pairs,
    weightA,
    weightB,
    alpha,
    beta,
    coverage_requirements,
):

    input_lines = []
    if has_diff:
        input_lines.append(str(len(color_pairs)))
        for (colorA,colorB,) in color_pairs:
            input_lines.append(
                f"{colorA} "
                f"{colorB} "
                f"{weightA} "
                f"{weightB} "
                f"{epsilon}"
            )

    else:
        input_lines.append("0")
    if has_ratio:
        input_lines.append(str(len(color_pairs)))
        for(colorA,colorB) in color_pairs:
            input_lines.append(
                f"{colorA} "
                f"{colorB} "
                f"{weightA} "
                f"{weightB} "
                f"{delta}"
            )
    else:
        input_lines.append(
            "0"
        )

    for q in queries:
        query_line = (
            f"{q['qx_min']} "
            f"{q['qx_max']} "
            f"{q['qy_min']} "
            f"{q['qy_max']} "
            f"{alpha} "
            f"{beta}"
        )

        if has_cov:
            query_line += (" "+ " ".join(str(value)for value in coverage_requirements))

        input_lines.append(
            query_line
        )


    # Exit query loop

    input_lines.append("-1")
    input_lines.append("-1")
    return ("\n".join(input_lines)+ "\n")



def parse_output(output,num_queries,):
    build_time = None
    query_times = []
    query_results = []
    build_match = re.search(
        r"Loaded in\s+([\d.]+)\s+ms",
        output,
    )

    if build_match:
        build_time = float(
            build_match.group(1)
        )

    rectangle_pattern = re.compile(
        r"Best rectangle\s*=\s*"
        r"\[([^,\]]+),([^\]]+)\]\s*x\s*"
        r"\[([^,\]]+),([^\]]+)\]\s*"
        r"\(similarity\s*=\s*([^)]+)\)"
    )

    for match in (rectangle_pattern.finditer(output)):
        x_min = (match.group(1).strip())
        x_max = (match.group(2).strip())
        y_min = (match.group(3).strip())
        y_max = (match.group(4).strip())
        similarity = (match.group(5).strip())
        query_results.append({
            "result":f"[{x_min},{x_max}] "f"x "f"[{y_min},{y_max}]",
            "result_x_min":x_min,
            "result_x_max":x_max,
            "result_y_min":y_min,
            "result_y_max":y_max,
            "similarity":similarity,
        })


    time_pattern = re.compile(r"Query executed in\s+"r"([\d.]+)\s+ms")

    for match in (time_pattern.finditer(output)):
        query_times.append(float(match.group(1)))



    while(len(query_times)< num_queries):
        query_times.append(None)
        
    while (len(query_results)< num_queries):
        query_results.append({})
    return (build_time,query_times[:num_queries],query_results[:num_queries])


if __name__ == "__main__":
    main()