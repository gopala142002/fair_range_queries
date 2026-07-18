import csv
import importlib.util
import multiprocessing as mp
import os
import re
import subprocess
import sys
import time
from pathlib import Path


DATASET = os.getenv("BENCH_DATASET", "synthetic_2d_40k.txt")
N_ROWS = int(os.getenv("BENCH_N", "40000"))
NUM_COLORS = 3

DIM_X = 0
DIM_Y = 1

ALPHA = 0.5
BETA = 0.5

EPSILONS = [10.0,20.0,50.0,100.0]
DELTAS = [0.05, 0.10, 0.20]

WEIGHT_A = 1.0
WEIGHT_B = 1.0

COVERAGE_REQUIREMENTS = [0, 5, 0]

COLOR_PAIRS = [("1", "2"),("1", "3"),("2", "3")]

COMBINATIONS = [
    (True,  False, False),
    (False, True,  False),
    (False, False, True),
    (True,  True,  False),
    (True,  False, True),
    (False, True,  True),
    (True,  True,  True),
]

REPETITIONS = int(os.getenv("BENCH_REPETITIONS", "1"))
CPP_TIMEOUT = int(os.getenv("BENCH_TIMEOUT", "1800"))
ILP_TIMEOUT = int(os.getenv("ILP_TIMEOUT", "1800"))

METHODS = [
    x.strip()
    # have to include bfs here
    for x in os.getenv("BENCH_METHODS","ilp,kd_opt",).split(",") if x.strip()
]

RESUME = os.getenv("BENCH_RESUME", "1") != "0"


def combination_name(has_diff, has_ratio, has_cov):
    parts = []

    if has_diff:
        parts.append("D")

    if has_ratio:
        parts.append("R")

    if has_cov:
        parts.append("C")

    return "+".join(parts)


def read_dataset(path, n_rows):
    points = []

    with open(path, "r") as f:
        declared_n = int(f.readline().strip())

        dim_line = f.readline().strip()

        match = re.search(r"(\d+)", dim_line)

        if not match:
            raise ValueError(f"Invalid dimension line: {dim_line}")

        rows_to_read = min(declared_n,n_rows,)

        for i in range(rows_to_read):
            line = f.readline()

            if not line:
                raise ValueError(f"Dataset ended at point {i}")

            parts = line.split()

            if len(parts) < 3:
                raise ValueError(f"Invalid dataset row: {line}")

            coords = [float(parts[0]),float(parts[1])]

            color = parts[2]

            points.append({"coords": [coords[DIM_X],coords[DIM_Y]],"color": color,"index": i})

    return points


def generate_queries(points):
    xs = sorted(p["coords"][0] for p in points)
    ys = sorted(p["coords"][1] for p in points)
    n = len(points)
    bucket_specs = [
        ("small", 0.10),
        ("medium", 0.40),
        ("large", 0.80),
    ]

    positions = [
        ("low", 0.0),
        ("mid", 0.5),
        ("high", 1.0),
    ]

    queries = []

    for bucket, fraction in bucket_specs:
        window = max(2,int(fraction * n),)
        max_start = n - window
        for pos, relative_start in positions:
            start_idx = int(round(relative_start* max_start))
            end_idx = min(n - 1,start_idx + window - 1)
            qx_min = xs[start_idx]
            qx_max = xs[end_idx]
            qy_min = ys[start_idx]
            qy_max = ys[end_idx]
            count = sum(
                1
                for p in points
                if (qx_min<= p["coords"][0]<= qx_max and qy_min<= p["coords"][1]<= qy_max)
            )

            queries.append({
                "query_id":f"{bucket}_{pos}",
                "qx_min": qx_min,
                "qx_max": qx_max,
                "qy_min": qy_min,
                "qy_max": qy_max,
                "point_count": count,
                "selectivity":100.0 * count / n,
                "bucket": bucket,
                "pos": pos,
            })

    return queries


def build_cpp_input(query,has_diff,has_ratio,has_cov,epsilon,delta,):
    lines = []

    if has_diff:
        lines.append(str(len(COLOR_PAIRS)))
        for color_a, color_b in COLOR_PAIRS:
            lines.append(
                f"{color_a} {color_b} "
                f"{WEIGHT_A} {WEIGHT_B} "
                f"{epsilon}"
            )
    else:
        lines.append("0")

    if has_ratio:
        lines.append(str(len(COLOR_PAIRS)))
        for color_a, color_b in COLOR_PAIRS:
            lines.append(
                f"{color_a} {color_b} "
                f"{WEIGHT_A} {WEIGHT_B} "
                f"{delta}"
            )
    else:
        lines.append("0")

    query_line = (
        f"{query['qx_min']} "
        f"{query['qx_max']} "
        f"{query['qy_min']} "
        f"{query['qy_max']} "
        f"{ALPHA} {BETA}"
    )

    if has_cov:
        query_line += (
            " "
            + " ".join(
                str(x)
                for x in COVERAGE_REQUIREMENTS
            )
        )

    lines.append(query_line)
    lines.append("-1")
    lines.append("-1")

    return "\n".join(lines) + "\n"


def parse_cpp_output(output):
    build_match = re.search(
        r"Loaded in\s+"
        r"([0-9.eE+-]+)\s+ms",
        output,
    )

    rect_match = re.search(
        r"Best rectangle\s*=\s*"
        r"\[([^,\]]+),([^\]]+)\]\s*x\s*"
        r"\[([^,\]]+),([^\]]+)\]\s*"
        r"\(similarity\s*=\s*"
        r"([0-9.eE+-]+)\)",
        output,
    )

    time_match = re.search(
        r"Query executed in\s+"
        r"([0-9.eE+-]+)\s+ms",
        output,
    )

    result = {
        "status": "NO_RESULT",
        "build_time_ms": None,
        "query_time_ms": None,
        "result": "[NA,NA] x [NA,NA]",
        "result_x_min": "NA",
        "result_x_max": "NA",
        "result_y_min": "NA",
        "result_y_max": "NA",
        "similarity": 0.0,
    }

    if build_match:
        result["build_time_ms"] = float(
            build_match.group(1)
        )

    if time_match:
        result["query_time_ms"] = float(
            time_match.group(1)
        )

    if rect_match:
        x_min = rect_match.group(1)
        x_max = rect_match.group(2)
        y_min = rect_match.group(3)
        y_max = rect_match.group(4)

        result.update({
            "status": "OK",
            "result":
                f"[{x_min},{x_max}] x "
                f"[{y_min},{y_max}]",
            "result_x_min": x_min,
            "result_x_max": x_max,
            "result_y_min": y_min,
            "result_y_max": y_max,
            "similarity":
                float(rect_match.group(5)),
        })

    return result


def run_cpp_method(executable,dataset_path,method,query,has_diff,has_ratio,has_cov,epsilon,delta,):
    if has_cov:
        mode_map = {"bfs": "bfs_cov","kd_opt": "kd_opt_cov"}
        run_mode = mode_map[method]
    else:
        run_mode = method

    input_text = build_cpp_input(query=query,has_diff=has_diff,has_ratio=has_ratio,has_cov=has_cov,epsilon=epsilon,delta=delta)

    cmd = [str(executable),str(dataset_path),str(NUM_COLORS),str(DIM_X),str(DIM_Y),run_mode,]

    try:
        completed = subprocess.run(cmd,input=input_text,text=True,capture_output=True,timeout=CPP_TIMEOUT,)

    except subprocess.TimeoutExpired:
        return run_mode, {
            "status": "TIMEOUT",
            "build_time_ms": None,
            "query_time_ms": None,
            "result": "[NA,NA] x [NA,NA]",
            "result_x_min": "NA",
            "result_x_max": "NA",
            "result_y_min": "NA",
            "result_y_max": "NA",
            "similarity": 0.0,
        }

    if completed.returncode != 0:
        print(completed.stdout)
        print(completed.stderr)

        return run_mode, {
            "status": "ERROR",
            "build_time_ms": None,
            "query_time_ms": None,
            "result": "[NA,NA] x [NA,NA]",
            "result_x_min": "NA",
            "result_x_max": "NA",
            "result_y_min": "NA",
            "result_y_max": "NA",
            "similarity": 0.0,
        }

    return (run_mode,parse_cpp_output(completed.stdout+ completed.stderr))


def load_ilp_solver(path):
    spec = importlib.util.spec_from_file_location("benchmark_ilp_solver",path,)

    if spec is None or spec.loader is None:
        raise RuntimeError(f"Cannot load ILP solver: {path}")

    module = importlib.util.module_from_spec(spec)

    spec.loader.exec_module(module)

    if not hasattr(module,"solve_fair_range_Dinkleback_warm_start",):
        raise RuntimeError("ILP solver does not contain ""solve_fair_range_Dinkleback_warm_start")

    return module


def prepare_ilp_points(points):
    colors = sorted({p["color"] for p in points})

    color_id = {color: i + 1 for i, color in enumerate(colors)}

    ilp_points = []

    for i, p in enumerate(points):
        ilp_points.append({"coords": list(p["coords"]),"index": i,"color_id":color_id[p["color"]],})

    return ilp_points, color_id


def run_ilp_method(ilp_solver,ilp_points,color_id,query,has_diff,has_ratio,has_cov,epsilon,delta,):
    qbox = [(query["qx_min"],query["qx_max"]),(query["qy_min"],query["qy_max"])]
    input_indices = set()
    for i, p in enumerate(ilp_points):
        x = p["coords"][0]
        y = p["coords"][1]

        if (query["qx_min"] <= x <= query["qx_max"] and query["qy_min"] <= y <= query["qy_max"]):
            input_indices.add(i)

    color_weights = {idx: 1.0 for idx in color_id.values()}

    diff_matrix = None

    if has_diff:
        diff_matrix = {}

        for a, b in COLOR_PAIRS:
            ca = color_id.get(a)
            cb = color_id.get(b)

            if ca is not None and cb is not None:
                diff_matrix[(ca, cb)] = epsilon

    ratio_matrix = None

    if has_ratio:
        ratio_matrix = {}

        for a, b in COLOR_PAIRS:
            ca = color_id.get(a)
            cb = color_id.get(b)

            if ca is not None and cb is not None:
                ratio_matrix[(ca, cb)] = delta

    min_coverage = None

    if has_cov:
        min_coverage = {}

        ordered_colors = sorted(color_id.items(),key=lambda item: item[0])

        for i, (_, cid) in enumerate(ordered_colors):
            min_coverage[cid] = (COVERAGE_REQUIREMENTS[i]
                if i < len(COVERAGE_REQUIREMENTS)
                else 0
            )

    start = time.perf_counter()

    try:
        gamma_approx, box_approx = (
            ilp_solver.solve_fair_range_Dinkleback_warm_start(ilp_points,input_indices,qbox,ALPHA,BETA,epsilon,d=2,delta_base=2,use_dynamic_grid=True,color_weights=color_weights,coverage=has_cov,min_coverage=min_coverage,diff_matrix=diff_matrix,ratio_matrix=ratio_matrix,))

        if box_approx is not None:
            if gamma_approx > 0:
                k_drop = int(len(input_indices)* (1 - gamma_approx))
                k_add = int(len(input_indices)* ((1.0 / gamma_approx)- 1))
            else:
                k_drop = None
                k_add = None

            gamma, box = (ilp_solver.solve_fair_range_Dinkleback_warm_start(ilp_points,input_indices,qbox,ALPHA,BETA,epsilon,d=2,delta_base=1,
use_dynamic_grid=False,start_gamma=gamma_approx,warm_start_box=box_approx,K_add=k_add,K_drop=k_drop,color_weights=color_weights,coverage=has_cov,min_coverage=min_coverage,diff_matrix=diff_matrix,ratio_matrix=ratio_matrix,))
        else:
            gamma, box = (gamma_approx,box_approx,)

    except Exception as exc:
        elapsed_ms = (time.perf_counter() - start) * 1000.0

        print(f"ILP ERROR: {exc}",flush=True,)

        return "ilp", {
            "status": "ERROR",
            "build_time_ms": None,
            "query_time_ms": elapsed_ms,
            "result": "[NA,NA] x [NA,NA]",
            "result_x_min": "NA",
            "result_x_max": "NA",
            "result_y_min": "NA",
            "result_y_max": "NA",
            "similarity": 0.0,
        }

    elapsed_ms = (
        time.perf_counter() - start
    ) * 1000.0

    if box is None:
        return "ilp", {
            "status": "NO_RESULT",
            "build_time_ms": None,
            "query_time_ms": elapsed_ms,
            "result": "[NA,NA] x [NA,NA]",
            "result_x_min": "NA",
            "result_x_max": "NA",
            "result_y_min": "NA",
            "result_y_max": "NA",
            "similarity": float(gamma),
        }

    x_min, x_max = box[0]
    y_min, y_max = box[1]

    return "ilp", {
        "status": "OK",
        "build_time_ms": None,
        "query_time_ms": elapsed_ms,
        "result":
            f"[{x_min},{x_max}] x "
            f"[{y_min},{y_max}]",
        "result_x_min": x_min,
        "result_x_max": x_max,
        "result_y_min": y_min,
        "result_y_max": y_max,
        "similarity": float(gamma),
    }



def _ilp_worker(queue, ilp_solver, ilp_points, color_id, query,
                has_diff, has_ratio, has_cov, epsilon, delta):
    """Run one ILP benchmark case in a child process."""
    try:
        result = run_ilp_method(
            ilp_solver, ilp_points, color_id, query,
            has_diff, has_ratio, has_cov, epsilon, delta
        )
        queue.put(result)
    except BaseException as exc:
        queue.put(("ilp", {
            "status": "ERROR",
            "build_time_ms": None,
            "query_time_ms": None,
            "result": "[NA,NA] x [NA,NA]",
            "result_x_min": "NA",
            "result_x_max": "NA",
            "result_y_min": "NA",
            "result_y_max": "NA",
            "similarity": 0.0,
        }))


def run_ilp_method_with_timeout(ilp_solver, ilp_points, color_id, query,
                                has_diff, has_ratio, has_cov, epsilon, delta):
    """
    Hard wall-clock timeout for the complete ILP call.
    On timeout, terminate the child process and return TIMEOUT.
    """
    # This benchmark is running on Linux; fork allows the dynamically loaded
    # ILP solver module to be inherited by the child process.
    ctx = mp.get_context("fork")
    queue = ctx.Queue(maxsize=1)

    process = ctx.Process(
        target=_ilp_worker,
        args=(
            queue, ilp_solver, ilp_points, color_id, query,
            has_diff, has_ratio, has_cov, epsilon, delta
        ),
    )

    process.start()
    process.join(ILP_TIMEOUT)

    if process.is_alive():
        print(
            f"ILP TIMEOUT after {ILP_TIMEOUT} seconds; terminating process...",
            flush=True,
        )
        process.terminate()
        process.join(10)

        if process.is_alive():
            process.kill()
            process.join()

        queue.close()
        queue.join_thread()

        return "ilp", {
            "status": "TIMEOUT",
            "build_time_ms": None,
            "query_time_ms": ILP_TIMEOUT * 1000.0,
            "result": "[NA,NA] x [NA,NA]",
            "result_x_min": "NA",
            "result_x_max": "NA",
            "result_y_min": "NA",
            "result_y_max": "NA",
            "similarity": 0.0,
        }

    try:
        result = queue.get(timeout=5)
    except Exception:
        result = ("ilp", {
            "status": "ERROR",
            "build_time_ms": None,
            "query_time_ms": None,
            "result": "[NA,NA] x [NA,NA]",
            "result_x_min": "NA",
            "result_x_max": "NA",
            "result_y_min": "NA",
            "result_y_max": "NA",
            "similarity": 0.0,
        })
    finally:
        queue.close()
        queue.join_thread()

    return result

def aggregate_results(results):
    successful = [r for r in results if r["status"] in ("OK","NO_RESULT",)]

    if not successful:
        status = ("TIMEOUT"if any(r["status"] == "TIMEOUT" for r in results) else "ERROR")

        return {"status": status,"iterations_done": 0,"build_time_ms": "N/A","avg_query_time_ms": "N/A","min_query_time_ms": "N/A","max_query_time_ms": "N/A",
"result": "[NA,NA] x [NA,NA]","result_x_min": "NA","result_x_max": "NA","result_y_min": "NA","result_y_max": "NA","similarity": 0.0,}

    times = [r["query_time_ms"] for r in successful if r["query_time_ms"] is not None]

    builds = [r["build_time_ms"] for r in successful if r["build_time_ms"] is not None]

    chosen = successful[0]

    return {
        "status": chosen["status"],
        "iterations_done":len(successful),
        "build_time_ms":
            f"{sum(builds) / len(builds):.4f}"
            if builds
            else "N/A",
        "avg_query_time_ms":
            f"{sum(times) / len(times):.4f}"
            if times
            else "N/A",
        "min_query_time_ms":
            f"{min(times):.4f}"
            if times
            else "N/A",
        "max_query_time_ms":
            f"{max(times):.4f}"
            if times
            else "N/A",
        "result": chosen["result"],
        "result_x_min":chosen["result_x_min"],
        "result_x_max":chosen["result_x_max"],
        "result_y_min":chosen["result_y_min"],
        "result_y_max":chosen["result_y_max"],
        "similarity":chosen["similarity"],
    }


def main():
    project_root = Path(os.getenv("PROJECT_ROOT",Path(__file__).resolve().parent.parent,)).resolve()

    dataset_path = (project_root/ "data"/ DATASET)

    executable = (project_root/ "bin"/ ("main_2d.exe" if sys.platform == "win32" else "main_2d"))

    ilp_solver_path = Path(os.getenv("ILP_SOLVER",str(project_root/ "src"/ "query_2d"/ "solve_ilp_dynamic_grid_dinkelbach_warm_start_bounded_optimized_copy.py"),)).resolve()

    output_csv = Path(os.getenv("BENCH_OUTPUT",str(project_root/ "scratch"/ "combinations_result_synthetic_2d.csv")))

    if not dataset_path.exists():
        raise FileNotFoundError(f"Dataset not found: {dataset_path}")

    if (any(m in ("bfs", "kd_opt") for m in METHODS) and not executable.exists()):
        raise FileNotFoundError(f"Executable not found: {executable}")

    ilp_solver = None

    if "ilp" in METHODS:
        if not ilp_solver_path.exists():
            raise FileNotFoundError(f"ILP solver not found: "f"{ilp_solver_path}")
        ilp_solver = load_ilp_solver(ilp_solver_path)

    points = read_dataset(dataset_path,N_ROWS,)

    n_rows = len(points)

    queries = generate_queries(points)

    ilp_points = None
    color_id = None

    if ilp_solver is not None:
        ilp_points, color_id = (prepare_ilp_points(points))

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

    output_csv.parent.mkdir(parents=True,exist_ok=True,)

    completed = set()

    if RESUME and output_csv.exists():
        with open(output_csv,"r",newline="",) as f:
            reader = csv.DictReader(f)
            for row in reader:
                completed.add((row.get("mode", ""),row.get("fairness_combination","",),row.get("epsilon", ""),row.get("delta", ""),row.get("query_id", ""),))

    file_mode = ("a" if RESUME and output_csv.exists() else "w")

    with open(output_csv,file_mode,newline="",buffering=1) as f:
        writer = csv.DictWriter(f,fieldnames=headers,)

        if file_mode == "w":
            writer.writeheader()

        for (has_diff,has_ratio,has_cov,) in COMBINATIONS:
            combo = combination_name(has_diff,has_ratio,has_cov,)

            eps_to_test = (EPSILONS if has_diff else [0.0])

            delta_to_test = (DELTAS if has_ratio else [0.0])

            num_diff = (len(COLOR_PAIRS) if has_diff else 0)

            num_ratio = (len(COLOR_PAIRS) if has_ratio else 0)
            num_cov = (1 if has_cov else 0)

            dimensions = (num_diff+ 2 * num_ratio)

            for epsilon in eps_to_test:
                for delta in delta_to_test:
                    for query in queries:
                        print(f"Combination: {combo} | "f"Query: "f"{query['query_id']} | "f"epsilon={epsilon} | "f"delta={delta}",flush=True)
                        for method in METHODS:
                            if method not in ("ilp","bfs","kd_opt"):
                                raise ValueError(f"Unknown method: "f"{method}")

                            if method == "ilp":
                                run_mode = "ilp"
                            elif has_cov:
                                run_mode = ("bfs_cov" if method == "bfs" else "kd_opt_cov")
                            else:
                                run_mode = method

                            row_key = (run_mode,combo,str(epsilon),str(delta),query["query_id"],)

                            if row_key in completed:
                                print(f"SKIP completed: "f"{row_key}",flush=True,)
                                continue

                            print(f"Running {run_mode}...",flush=True)

                            runs = []

                            for _ in range(REPETITIONS):
                                if method == "ilp":
                                    _, result = run_ilp_method_with_timeout(ilp_solver,ilp_points,color_id,query,has_diff,has_ratio,has_cov,epsilon,delta)
                                else:
                                    _, result = run_cpp_method(executable,dataset_path,method,query,has_diff,has_ratio,has_cov,epsilon,delta)

                                runs.append(result)

                            aggregate = aggregate_results(runs)

                            row = {
                                "dataset": DATASET,
                                "n": n_rows,
                                "colors": NUM_COLORS,
                                "algorithm": method,
                                "mode": run_mode,
                                "fairness_combination":combo,
                                "epsilon": epsilon,
                                "delta": delta,
                                "weightA": WEIGHT_A,
                                "weightB": WEIGHT_B,
                                "alpha": ALPHA,
                                "beta": BETA,
                                "num_diff_constraints":num_diff,
                                "num_ratio_constraints":num_ratio,
                                "num_coverage_constraints":num_cov,
                                "dimensions": dimensions,
                                "dim_x": DIM_X,
                                "dim_y": DIM_Y,
                                "qx_min":query["qx_min"],
                                "qx_max":query["qx_max"],
                                "qy_min":query["qy_min"],
                                "qy_max":query["qy_max"],
                                "query_id":query["query_id"],
                                "query_point_count":query["point_count"],
                                "query_selectivity_percent":f"{query['selectivity']:.4f}",
                                "range_bucket":query["bucket"],
                                "range_position":query["pos"],
                                "iterations_done":aggregate["iterations_done"],
                                "build_time_ms":aggregate["build_time_ms"],
                                "avg_query_time_ms":aggregate["avg_query_time_ms"],
                                "min_query_time_ms":aggregate["min_query_time_ms"],
                                "max_query_time_ms":aggregate["max_query_time_ms"],
                                "result":aggregate["result"],
                                "result_x_min":aggregate["result_x_min"],
                                "result_x_max":aggregate["result_x_max"],
                                "result_y_min":aggregate["result_y_min"],
                                "result_y_max":aggregate["result_y_max"],
                                "similarity":aggregate["similarity"],
                                "tle":"YES" if aggregate["status"] == "TIMEOUT" else "NO",
                            }

                            writer.writerow(row)
                            f.flush()

                            completed.add(row_key)

                            print(f"SAVED: "f"mode={run_mode} "f"combo={combo} "f"query="f"{query['query_id']} "f"avg_ms="f"{aggregate['avg_query_time_ms']} "f"similarity="f"{aggregate['similarity']} "f"status="f"{aggregate['status']}",flush=True)

    print(f"\nDone. Results saved to "f"{output_csv}",flush=True)


if __name__ == "__main__":
    main()