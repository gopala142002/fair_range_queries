import csv
import random
import re
import subprocess
import sys
from pathlib import Path


# ============================================================
# PROJECT PATHS
# ============================================================

PROJECT_ROOT = Path(__file__).resolve().parent.parent

DATASET = PROJECT_ROOT / "data" / "live_l2_5d_30k.txt"

OUTPUT_FILE = (
    PROJECT_ROOT
    / "scratch"
    / "combinations_result_2d.csv"
)

EXE_EXT = ".exe" if sys.platform == "win32" else ""

EXECUTABLE = (
    PROJECT_ROOT
    / "bin"
    / f"main_2d{EXE_EXT}"
)


# ============================================================
# EXPERIMENT CONFIGURATION
# ============================================================

NUM_COLORS = 3

DIM_X = 0
DIM_Y = 1

ALPHA = 0.5
BETA = 0.5

WEIGHT_A = 1.0
WEIGHT_B = 1.0

REPETITIONS = 3

RANDOM_SEED = 42


# ------------------------------------------------------------
# Algorithms
# ------------------------------------------------------------

MODES = [
    "kd_opt",
    "bfs",

    # Do not enable for 30K unless you really want to.
    # "brute",
]


# ------------------------------------------------------------
# Fairness parameters
# ------------------------------------------------------------

EPSILONS = [
    0.0,
    2.0,
    10.0,
    100.0,
]

DELTAS = [
    0.0,
    0.05,
    0.10,
    0.20,
]


# ------------------------------------------------------------
# Fairness combinations
#
# D     = Difference only
# R     = Ratio only
# D+R   = Difference + Ratio
# ------------------------------------------------------------

COMBINATIONS = [
    (True, False),
    (False, True),
    (True, True),
]


# ============================================================
# READ ORIGINAL 5D DATASET
# ============================================================

def load_5d_dataset(dataset_path):
    """
    Expected input format:

        30000
        5 Dims
        d0 d1 d2 d3 d4
        d0 d1 d2 d3 d4
        ...
        colors...

    Returns:
        n
        dimensions
        points
    """

    with open(dataset_path, "r") as f:

        first_line = f.readline().strip()

        if not first_line:
            raise ValueError("Dataset is empty.")

        n = int(first_line)

        dimension_line = f.readline().strip()

        match = re.search(r"(\d+)", dimension_line)

        if not match:
            raise ValueError(
                f"Cannot parse dimension line: {dimension_line}"
            )

        num_dimensions = int(match.group(1))

        points = []

        for i in range(n):

            line = f.readline()

            if not line:
                raise ValueError(
                    f"Expected {n} points, but file ended at point {i}"
                )

            values = list(map(float, line.split()))

            if len(values) < num_dimensions:
                raise ValueError(
                    f"Point {i} has only {len(values)} dimensions."
                )

            points.append(values)

    return n, num_dimensions, points


# ============================================================
# QUERY GENERATION
# ============================================================

def generate_queries(points, dim_x, dim_y):
    """
    Generate 9 rectangular queries:

        3 Small
        3 Medium
        3 Large

    The query rectangle boundaries are generated from coordinate
    quantiles.

    Small  : approximately 10% coordinate span per dimension
    Medium : approximately 40%
    Large  : approximately 80%
    """

    xs = sorted(point[dim_x] for point in points)
    ys = sorted(point[dim_y] for point in points)

    n = len(points)

    bucket_configs = [
        ("S", 0.10),
        ("M", 0.40),
        ("L", 0.80),
    ]

    queries = []

    for bucket, fraction in bucket_configs:

        window_size = max(
            2,
            int(n * fraction)
        )

        for query_number in range(3):

            x_start = random.randint(
                0,
                n - window_size
            )

            y_start = random.randint(
                0,
                n - window_size
            )

            x_end = x_start + window_size - 1
            y_end = y_start + window_size - 1

            qx_min = xs[x_start]
            qx_max = xs[x_end]

            qy_min = ys[y_start]
            qy_max = ys[y_end]


            # Position classification based on X midpoint

            midpoint = (x_start + x_end) / 2

            if midpoint < n / 3:
                position = "low"

            elif midpoint > 2 * n / 3:
                position = "high"

            else:
                position = "mid"


            queries.append({

                "query_id":
                    f"{bucket}_{query_number + 1}",

                "bucket":
                    bucket,

                "position":
                    position,

                "qx_min":
                    qx_min,

                "qx_max":
                    qx_max,

                "qy_min":
                    qy_min,

                "qy_max":
                    qy_max,
            })

    return queries


# ============================================================
# BUILD CONSTRAINT INPUT
# ============================================================

def build_constraint_lines(
    has_diff,
    has_ratio,
    epsilon,
    delta
):
    """
    Constraints used:

    Difference constraints:
        1 vs 2
        1 vs 3
        2 vs 3

    Ratio constraints:
        1 vs 2
        1 vs 3
        2 vs 3
    """

    lines = []


    # --------------------------------------------------------
    # Difference constraints
    # --------------------------------------------------------

    if has_diff:

        diff_pairs = [
            (1, 2),
            (1, 3),
            (2, 3),
        ]

        lines.append(
            str(len(diff_pairs))
        )

        for color_a, color_b in diff_pairs:

            lines.append(

                f"{color_a} "
                f"{color_b} "
                f"{WEIGHT_A} "
                f"{WEIGHT_B} "
                f"{epsilon}"
            )

    else:

        lines.append("0")


    # --------------------------------------------------------
    # Ratio constraints
    # --------------------------------------------------------

    if has_ratio:

        ratio_pairs = [
            (1, 2),
            (1, 3),
            (2, 3),
        ]

        lines.append(
            str(len(ratio_pairs))
        )

        for color_a, color_b in ratio_pairs:

            lines.append(

                f"{color_a} "
                f"{color_b} "
                f"{WEIGHT_A} "
                f"{WEIGHT_B} "
                f"{delta}"
            )

    else:

        lines.append("0")


    return lines


# ============================================================
# BUILD COMPLETE STDIN
# ============================================================

def build_program_input(
    queries,
    has_diff,
    has_ratio,
    epsilon,
    delta
):

    input_lines = build_constraint_lines(

        has_diff,
        has_ratio,
        epsilon,
        delta
    )


    # --------------------------------------------------------
    # Add all queries
    # --------------------------------------------------------

    for query in queries:

        input_lines.append(

            f"{query['qx_min']} "
            f"{query['qx_max']} "
            f"{query['qy_min']} "
            f"{query['qy_max']} "
            f"{ALPHA} "
            f"{BETA}"
        )


    # --------------------------------------------------------
    # Exit query loop
    # --------------------------------------------------------

    input_lines.append("-1")


    # --------------------------------------------------------
    # Exit outer constraint loop
    # --------------------------------------------------------

    input_lines.append("-1")


    return "\n".join(input_lines) + "\n"


# ============================================================
# OUTPUT PARSER
# ============================================================

def parse_output(output, num_queries):

    build_time = None

    query_times = []

    query_results = []


    # --------------------------------------------------------
    # Parse build/load time
    #
    # Example:
    #
    # 2D kd_opt mode. Loaded in 25.1234 ms
    # --------------------------------------------------------

    build_match = re.search(

        r"Loaded in\s+"
        r"([0-9]+(?:\.[0-9]+)?)"
        r"\s+ms",

        output
    )

    if build_match:

        build_time = float(
            build_match.group(1)
        )


    # --------------------------------------------------------
    # Parse rectangles
    #
    # Example:
    #
    # Best rectangle =
    # [x1,x2] x [y1,y2] (similarity = 0.95)
    # --------------------------------------------------------

    rectangle_pattern = re.compile(

        r"Best rectangle\s*=\s*"

        r"\[([^,\]]+),([^\]]+)\]"

        r"\s*x\s*"

        r"\[([^,\]]+),([^\]]+)\]"

        r"\s*"

        r"\(similarity\s*=\s*([^)]+)\)"
    )


    for match in rectangle_pattern.finditer(output):

        query_results.append({

            "result_x_min":
                match.group(1).strip(),

            "result_x_max":
                match.group(2).strip(),

            "result_y_min":
                match.group(3).strip(),

            "result_y_max":
                match.group(4).strip(),

            "similarity":
                match.group(5).strip(),

            "result":
                (
                    f"[{match.group(1).strip()},"
                    f"{match.group(2).strip()}]"
                    f"x"
                    f"[{match.group(3).strip()},"
                    f"{match.group(4).strip()}]"
                )
        })


    # --------------------------------------------------------
    # Parse query times
    #
    # Example:
    #
    # Query executed in 12.3456 ms
    # --------------------------------------------------------

    time_pattern = re.compile(

        r"Query executed in\s+"
        r"([0-9]+(?:\.[0-9]+)?)"
        r"\s+ms"
    )


    for match in time_pattern.finditer(output):

        query_times.append(
            float(match.group(1))
        )


    # --------------------------------------------------------
    # Ensure fixed lengths
    # --------------------------------------------------------

    while len(query_times) < num_queries:

        query_times.append(None)


    while len(query_results) < num_queries:

        query_results.append({

            "result":
                "[NA,NA]x[NA,NA]",

            "result_x_min":
                "NA",

            "result_x_max":
                "NA",

            "result_y_min":
                "NA",

            "result_y_max":
                "NA",

            "similarity":
                "0.0",
        })


    return (
        build_time,
        query_times[:num_queries],
        query_results[:num_queries]
    )


# ============================================================
# FAIRNESS COMBINATION NAME
# ============================================================

def get_combination_name(
    has_diff,
    has_ratio
):

    names = []

    if has_diff:
        names.append("D")

    if has_ratio:
        names.append("R")

    return "+".join(names)


# ============================================================
# RUN ONE CONFIGURATION
# ============================================================

def run_configuration(
    mode,
    queries,
    has_diff,
    has_ratio,
    epsilon,
    delta
):

    input_data = build_program_input(

        queries,

        has_diff,
        has_ratio,

        epsilon,
        delta
    )


    command = [

        str(EXECUTABLE),

        str(DATASET),

        str(NUM_COLORS),

        str(DIM_X),

        str(DIM_Y),

        mode,
    ]


    all_build_times = []

    all_query_times = [

        []
        for _ in queries
    ]

    stored_results = [

        None
        for _ in queries
    ]

    successful_runs = 0


    for repetition in range(REPETITIONS):

        print(

            f"      Run "
            f"{repetition + 1}/"
            f"{REPETITIONS}"
        )


        try:

            process = subprocess.run(

                command,

                input=input_data,

                capture_output=True,

                text=True,

                timeout=3600
            )


        except subprocess.TimeoutExpired:

            print(
                f"      TIMEOUT: {mode}"
            )

            continue


        if process.returncode != 0:

            print(
                f"      ERROR: {mode}"
            )

            print(process.stderr)

            continue


        successful_runs += 1


        (
            build_time,
            query_times,
            query_results

        ) = parse_output(

            process.stdout,

            len(queries)
        )


        if build_time is not None:

            all_build_times.append(
                build_time
            )


        for query_index in range(
            len(queries)
        ):

            query_time = (
                query_times[query_index]
            )


            if query_time is not None:

                all_query_times[
                    query_index
                ].append(
                    query_time
                )


            if (
                stored_results[query_index]
                is None
            ):

                stored_results[
                    query_index
                ] = query_results[
                    query_index
                ]


    return {

        "successful_runs":
            successful_runs,

        "build_times":
            all_build_times,

        "query_times":
            all_query_times,

        "results":
            stored_results,
    }


# ============================================================
# MAIN
# ============================================================

def main():

    random.seed(RANDOM_SEED)


    # --------------------------------------------------------
    # Validate paths
    # --------------------------------------------------------

    if not DATASET.exists():

        print(
            f"Dataset not found:\n{DATASET}"
        )

        return


    if not EXECUTABLE.exists():

        print(
            f"Executable not found:\n{EXECUTABLE}"
        )

        print(
            "\nRun:\n"
            "make two_d"
        )

        return


    # --------------------------------------------------------
    # Load dataset
    # --------------------------------------------------------

    print("\nLoading dataset...")

    (
        n,
        num_dimensions,
        points

    ) = load_5d_dataset(DATASET)


    print(
        f"Points: {n}"
    )

    print(
        f"Original dimensions: {num_dimensions}"
    )

    print(
        f"2D projection: ({DIM_X}, {DIM_Y})"
    )


    # --------------------------------------------------------
    # Generate query rectangles
    # --------------------------------------------------------

    queries = generate_queries(

        points,

        DIM_X,

        DIM_Y
    )


    print("\nGenerated 2D queries:\n")


    for query in queries:

        print(

            f"{query['query_id']:5s} "

            f"X=["
            f"{query['qx_min']}, "
            f"{query['qx_max']}] "

            f"Y=["
            f"{query['qy_min']}, "
            f"{query['qy_max']}]"
        )


    # --------------------------------------------------------
    # CSV header
    # --------------------------------------------------------

    headers = [

        "dataset",

        "n",

        "colors",

        "algorithm",

        "fairness_combination",

        "epsilon",

        "delta",

        "weightA",

        "weightB",

        "alpha",

        "beta",

        "num_diff_constraints",

        "num_ratio_constraints",

        "tree_dimensions",

        "dim_x",

        "dim_y",

        "query_id",

        "qx_min",

        "qx_max",

        "qy_min",

        "qy_max",

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


    OUTPUT_FILE.parent.mkdir(
        parents=True,
        exist_ok=True
    )


    # --------------------------------------------------------
    # Start experiments
    # --------------------------------------------------------

    with open(

        OUTPUT_FILE,

        "w",

        newline=""

    ) as csv_file:


        writer = csv.DictWriter(

            csv_file,

            fieldnames=headers
        )


        writer.writeheader()


        # ====================================================
        # FAIRNESS COMBINATIONS
        # ====================================================

        for (
            has_diff,
            has_ratio

        ) in COMBINATIONS:


            combination_name = (
                get_combination_name(
                    has_diff,
                    has_ratio
                )
            )


            epsilon_values = (

                EPSILONS

                if has_diff

                else [0.0]
            )


            delta_values = (

                DELTAS

                if has_ratio

                else [0.0]
            )


            num_diff_constraints = (

                3 if has_diff else 0
            )


            num_ratio_constraints = (

                3 if has_ratio else 0
            )


            tree_dimensions = (

                num_diff_constraints

                +

                2 * num_ratio_constraints
            )


            # ================================================
            # EPSILON
            # ================================================

            for epsilon in epsilon_values:


                # ============================================
                # DELTA
                # ============================================

                for delta in delta_values:


                    print(
                        "\n"
                        "=========================================="
                    )

                    print(
                        f"Combination : {combination_name}"
                    )

                    print(
                        f"Epsilon     : {epsilon}"
                    )

                    print(
                        f"Delta       : {delta}"
                    )

                    print(
                        "=========================================="
                    )


                    # ========================================
                    # ALGORITHMS
                    # ========================================

                    for mode in MODES:


                        print(
                            f"\n   Running {mode}"
                        )


                        run_data = run_configuration(

                            mode,

                            queries,

                            has_diff,

                            has_ratio,

                            epsilon,

                            delta
                        )


                        successful_runs = (

                            run_data[
                                "successful_runs"
                            ]
                        )


                        if successful_runs == 0:

                            print(
                                f"   No successful runs "
                                f"for {mode}"
                            )

                            continue


                        # ------------------------------------
                        # Build time
                        # ------------------------------------

                        build_times = (

                            run_data[
                                "build_times"
                            ]
                        )


                        if build_times:

                            avg_build_time = (

                                sum(build_times)

                                /

                                len(build_times)
                            )

                        else:

                            avg_build_time = None


                        # ------------------------------------
                        # Write one row per query
                        # ------------------------------------

                        for query_index, query in enumerate(
                            queries
                        ):


                            times = (

                                run_data[
                                    "query_times"
                                ][query_index]
                            )


                            result = (

                                run_data[
                                    "results"
                                ][query_index]
                            )


                            if result is None:

                                result = {

                                    "result":
                                        "[NA,NA]x[NA,NA]",

                                    "result_x_min":
                                        "NA",

                                    "result_x_max":
                                        "NA",

                                    "result_y_min":
                                        "NA",

                                    "result_y_max":
                                        "NA",

                                    "similarity":
                                        "0.0",
                                }


                            if times:

                                avg_query_time = (

                                    sum(times)

                                    /

                                    len(times)
                                )

                                min_query_time = min(
                                    times
                                )

                                max_query_time = max(
                                    times
                                )

                            else:

                                avg_query_time = None

                                min_query_time = None

                                max_query_time = None


                            row = {

                                "dataset":
                                    DATASET.name,

                                "n":
                                    n,

                                "colors":
                                    NUM_COLORS,

                                "algorithm":
                                    mode,

                                "fairness_combination":
                                    combination_name,

                                "epsilon":
                                    epsilon,

                                "delta":
                                    delta,

                                "weightA":
                                    WEIGHT_A,

                                "weightB":
                                    WEIGHT_B,

                                "alpha":
                                    ALPHA,

                                "beta":
                                    BETA,

                                "num_diff_constraints":
                                    num_diff_constraints,

                                "num_ratio_constraints":
                                    num_ratio_constraints,

                                "tree_dimensions":
                                    tree_dimensions,

                                "dim_x":
                                    DIM_X,

                                "dim_y":
                                    DIM_Y,

                                "query_id":
                                    query["query_id"],

                                "qx_min":
                                    query["qx_min"],

                                "qx_max":
                                    query["qx_max"],

                                "qy_min":
                                    query["qy_min"],

                                "qy_max":
                                    query["qy_max"],

                                "range_bucket":
                                    query["bucket"],

                                "range_position":
                                    query["position"],

                                "iterations_done":
                                    successful_runs,

                                "build_time_ms":
                                    (
                                        f"{avg_build_time:.4f}"
                                        if avg_build_time
                                        is not None
                                        else "N/A"
                                    ),

                                "avg_query_time_ms":
                                    (
                                        f"{avg_query_time:.4f}"
                                        if avg_query_time
                                        is not None
                                        else "N/A"
                                    ),

                                "min_query_time_ms":
                                    (
                                        f"{min_query_time:.4f}"
                                        if min_query_time
                                        is not None
                                        else "N/A"
                                    ),

                                "max_query_time_ms":
                                    (
                                        f"{max_query_time:.4f}"
                                        if max_query_time
                                        is not None
                                        else "N/A"
                                    ),

                                "result":
                                    result["result"],

                                "result_x_min":
                                    result["result_x_min"],

                                "result_x_max":
                                    result["result_x_max"],

                                "result_y_min":
                                    result["result_y_min"],

                                "result_y_max":
                                    result["result_y_max"],

                                "similarity":
                                    result["similarity"],

                                "tle":
                                    "NO",
                            }


                            writer.writerow(row)


                            # Save continuously so results survive
                            # Ctrl+C or system interruption.

                            csv_file.flush()


                        print(
                            f"   Finished {mode}"
                        )


    print(
        "\n=========================================="
    )

    print(
        "2D benchmarking complete."
    )

    print(
        f"Results saved to:\n{OUTPUT_FILE}"
    )

    print(
        "=========================================="
    )


if __name__ == "__main__":
    main()