import subprocess
import re

EXE = "./bin/main_2d"
DATASET = "data/live_l2_2d_100.txt"

queries = {
    "small_mid":  (42.453, 94.803, 69.568, 109.186),
    "medium_low": (0.03, 23.725, 0.349, 53.011),
    "large_mid":  (1.87, 1829.18, 4.562, 1735.83),
}

epsilons = [10.0]

modes = ["brute", "bfs"]

epsilons = [10.0]

modes = ["brute", "bfs"]


def run(mode, query, epsilon):

    xmin, xmax, ymin, ymax = query

    input_text = f"""3
1 2 1.0 1.0 {epsilon}
1 3 1.0 1.0 {epsilon}
2 3 1.0 1.0 {epsilon}
0
{xmin} {xmax} {ymin} {ymax} 0.5 0.5
-1
-1
"""

    cmd = [
        EXE,
        DATASET,
        "3",
        "0",
        "1",
        mode,
    ]

    try:
        result = subprocess.run(
            cmd,
            input=input_text,
            text=True,
            capture_output=True,
            timeout=120,
        )

    except subprocess.TimeoutExpired:
        return {
            "status": "TIMEOUT",
            "similarity": None,
            "time_ms": None,
        }

    output = result.stdout + result.stderr

    sim_match = re.search(
        r"similarity\s*=\s*([0-9.eE+-]+)",
        output
    )

    time_match = re.search(
        r"Query executed in\s+([0-9.eE+-]+)\s+ms",
        output
    )

    if not sim_match:
        return {
            "status": "NO_RESULT",
            "similarity": None,
            "time_ms": None,
        }

    similarity = float(sim_match.group(1))

    time_ms = (
        float(time_match.group(1))
        if time_match
        else None
    )

    return {
        "status": "OK",
        "similarity": similarity,
        "time_ms": time_ms,
    }


for query_name, query in queries.items():

    for epsilon in epsilons:

        print(
            f"\n========================================\n"
            f"Query: {query_name}, epsilon={epsilon}\n"
            f"========================================"
        )

        results = {}

        for mode in modes:

            print(f"Running {mode}...", flush=True)

            results[mode] = run(
                mode,
                query,
                epsilon,
            )

            print(
                mode,
                results[mode]
            )

        brute = results["brute"]
        kd = results["bfs"]

        if (
            brute["status"] == "OK"
            and kd["status"] == "OK"
        ):

            difference = abs(
                brute["similarity"]
                - kd["similarity"]
            )

            if difference <= 1e-4:
                print("RESULT: PASS")
            else:
                print(
                    "RESULT: MISMATCH",
                    "difference =",
                    difference
                )

        else:
            print("RESULT: INCOMPLETE")