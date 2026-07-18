import os
import csv
import copy
import random
import numpy as np
from itertools import combinations


DATASET = "live_l2_5d_30k_with_color.txt"
DIMENSIONS = [2,3,4,5]
NUM_QUERIES_PER_BUCKET = 1
NUM_COLORS = 5
BUCKETS = {
    "b1": (0.00,0.10),
    "b2": (0.10,0.40),
    "b3": (0.40,1.00)
}

random.seed(42)
np.random.seed(42)


def load_dataset(filename):
    with open(filename) as f:
        lines = f.readlines()
    n = int(lines[0].strip())
    coords=[]
    colors=[]
    for line in lines[2:]:
        vals=line.split()
        if len(vals)==6:
            coords.append(list(map(float,vals[:5])))
            colors.append(int(vals[5]))
    return n,np.array(coords),colors



def generate_queries(coords,n,d):
    buckets={}
    for b in BUCKETS:
        buckets[b]=[]
    attempts=0
    while True:
        attempts+=1
        qbox=[]
        for k in range(d):
            vals=coords[:,k]
            low=random.uniform(0,40)
            high=random.uniform(60,100)
            if random.random()<0.20:
                low=random.uniform(0,10)
                high=random.uniform(90,100)

            mn=float(np.percentile(vals,low))
            mx=float(np.percentile(vals,high))
            qbox.append((mn,mx))
        indices=[]
        for i,p in enumerate(coords):
            ok=True
            for k in range(d):
                if p[k]<qbox[k][0] or p[k]>qbox[k][1]:
                    ok=False
                    break
            if ok:
                indices.append(i)
        cnt=len(indices)
        ratio=cnt/n
        for bucket in BUCKETS:
            lo,hi=BUCKETS[bucket]
            if lo<=ratio<=hi:
                if len(buckets[bucket])<NUM_QUERIES_PER_BUCKET:
                    buckets[bucket].append(
                        (
                            qbox,
                            set(indices),
                            cnt
                        )
                    )
        done=True
        for b in buckets:
            if len(buckets[b])<NUM_QUERIES_PER_BUCKET:
                done=False
        if done:
            break
        if attempts>10000:
            break
    return buckets



def generate_constraints(qbox,input_indices,coords,colors,d):
    ranges=[]
    for i in range(d):
        ranges.append(qbox[i][1]-qbox[i][0])
    candidates=[]
    for _ in range(2000):
        box=[]
        for k in range(d):
            s1=random.uniform(-0.15,0.15)*ranges[k]
            s2=random.uniform(-0.15,0.15)*ranges[k]
            a=qbox[k][0]+s1
            b=qbox[k][1]+s2
            box.append((min(a,b),max(a,b)))
        ids=set()
        for i,p in enumerate(coords):
            ok=True
            for k in range(d):
                if p[k]<box[k][0] or p[k]>box[k][1]:
                    ok=False
                    break
            if ok:
                ids.add(i)
        inter=len(ids & input_indices)
        uni=len(ids | input_indices)
        sim=0
        if uni>0:
            sim=inter/uni
        candidates.append((box,ids,sim))
    candidates.sort(key=lambda x:abs(x[2]-0.50))
    target_box,target_ids,sim=candidates[0]
    counts={}
    for c in range(1,NUM_COLORS+1):
        counts[c]=0
    for idx in target_ids:
        counts[colors[idx]]+=1

    coverage = ["0"] * NUM_COLORS

    cov = random.sample(range(1, NUM_COLORS + 1), 3)

    for c in cov:
        coverage[c - 1] = str(counts[c])


    diff = [["NaN"] * NUM_COLORS for _ in range(NUM_COLORS)]

    pairs = list(combinations(range(1, NUM_COLORS + 1), 2))

    diff_pairs = random.sample(pairs, 3)

    for a, b in diff_pairs:
        diff[a - 1][b - 1] = str(abs(counts[a] - counts[b]))


    ratio = [["NaN"] * NUM_COLORS for _ in range(NUM_COLORS)]

    rem = [p for p in pairs if p not in diff_pairs]

    ratio_pairs = random.sample(rem, 3)

    for a, b in ratio_pairs:
        if counts[a] > 0 and counts[b] > 0:
            r = max(counts[a] / counts[b],
                    counts[b] / counts[a])
            ratio[a - 1][b - 1] = str(round(r, 4))
        

    

    text = "Weights of color:\n"
    text += " ".join(["1.0"]*NUM_COLORS)
    text += "\n\n"

    text += "Coverage:\n"
    text += " ".join(coverage)
    text += "\n\n"

    text += "Difference:\n"

    for row in diff:
        text += " ".join(row)
        text += "\n"

    text += "\nRatio:\n"

    for row in ratio:
        text += " ".join(row)
        text += "\n"

    return text, target_box, sim



SOLVERS = [
    {
        "name": "Dinkelbach_WS_Grid",
        "cmd": [
            "python3",
            "ilp_solvers_corrected/solve_ilp_dynamic_grid_dinkelbach_warm_start_bounded_optimized_validpts.py"
        ]
    },
    {
        "name": "Dinkelbach_NoWS",
        "cmd": [
            "python3",
            "ilp_solvers_corrected/solve_ilp_dinkelbach_no_dynamic_grid_bounded_optimized_validpts.py"
        ]
    },
    {
        "name": "BinarySearch_NoGrid",
        "cmd": [
            "python3",
            "ilp_solvers_corrected/solve_ilp_binary_search_no_dynamic_grid_bounded_optimized_validpts.py"
        ]
    },
    {
        "name": "BinarySearch_WS_Grid",
        "cmd": [
            "python3",
            "ilp_solvers_corrected/solve_ilp_dynamic_grid_binary_search_warm_start_bounded_optimized_validpts.py"
        ]
    },
    {
        "name":"BFSMP",
        "cmd":["./bfsmp"]
    }
]

def write_temp_dataset(filename,n,d,coords,colors,qbox):
    with open(filename, "w") as f:
        f.write(f"{n}\n")
        f.write(f"{d}\n")
        f.write("0\n")        # t = 0

        # Coordinates only
        for p in coords:
            vals = p[:d]
            f.write(" ".join(map(str, vals)) + "\n")

        # Colors separately
        for c in colors:
            f.write(f"{c}\n")

        # Query
        for lo, hi in qbox:
            f.write(f"{lo} {hi}\n")

def write_constraint_file(filename,text):

    with open(filename,"w") as f:

        f.write(text)



import subprocess
import time
import re

def run_solver(solver,
               data_file,
               constraint_file):

    cmd=solver["cmd"]+[data_file,constraint_file]

    runtime="N/A"
    similarity="N/A"
    status="Success"

    try:

        process=subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )

        stdout, _ = process.communicate(timeout=1800)

        print(stdout, end="")

        text = stdout

        if solver["name"]=="BFSMP":

            m=re.search(
                r"Execution time:\s*([0-9.eE+-]+)",
                text
            )

            if m:
                runtime=float(m.group(1))

            m=re.search(
                r"Max sim:\s*([0-9.eE+-]+)",
                text
            )
            if m:
                similarity=float(m.group(1))

        else:
            m=re.search(
                r"Time taken to find fair range:\s*([0-9.]+)",
                text
            )

            if m:
                runtime=float(m.group(1))

            m=re.search(
                r"Jaccard Similarity:\s*([0-9.]+)",
                text
            )
            if m:
                similarity=float(m.group(1))

        if "No feasible box found" in text:
            status="Infeasible"

        if process.returncode!=0:
            status="Error"

    except subprocess.TimeoutExpired:
        process.kill()

        stdout, _ = process.communicate()
        print(stdout, end="")

        runtime = "TIMEOUT"
        similarity = "TIMEOUT"
        status = "Timeout"

    except Exception:
        runtime="FAIL"
        similarity="FAIL"
        status="Fail"
    return runtime,similarity,status


CSV_FILE="ablation_result_livel21_new_for_sim1_b2.csv"

def create_csv():
    if os.path.exists(CSV_FILE):
        return

    with open(CSV_FILE,"w",newline="") as f:
        writer=csv.writer(f)
        writer.writerow([
            "Dimension",
            "Bucket",
            "Query",
            "InputPoints",
            "QueryBox",
            "TargetBox",
            "TargetSimilarity",
            "Constraints",
            "Solver",
            "Runtime",
            "Similarity",
            "Status"
        ])


def append_csv(dimension,bucket,query,input_points,query_box,target_box,target_similarity,constraints,solver,runtime,similarity,status):

    with open(CSV_FILE,"a",newline="") as f:
        writer=csv.writer(f)
        writer.writerow(
            [
                dimension,
                bucket,
                query,
                input_points,
                str(query_box),
                str(target_box),
                target_similarity,
                constraints,
                solver,
                runtime,
                similarity,
                status
            ]
        )


####

def main():
    create_csv()
    if not os.path.exists("bfsmp"):
        print("Compiling bfsmp...")
        subprocess.run(
            [
                "g++",
                "-O3",
                "bfsmp.cpp",
                "-o",
                "bfsmp"
            ],
            check=True
        )

    n,coords,colors=load_dataset(DATASET)
    print()
    print("DATASET LOADED")
    print()

    query_id=1

    for dimension in DIMENSIONS:
        query_id = 1
        print()
        print(f"RUNNING {dimension}D BENCHMARK")
        buckets=generate_queries(coords,n,dimension)
        for bucket in buckets:
            print()
            print(bucket)
            for qbox,input_indices,input_points in buckets[bucket]:
                print()
                print("Generating fairness constraints...")
                constraints,target_box,target_sim=generate_constraints(
                    qbox,
                    input_indices,
                    coords,
                    colors,
                    dimension
                )


                constraint_file=f"constraint_d{dimension}_{bucket}_q{query_id}.txt"
                write_constraint_file(constraint_file,constraints)
                query_file = f"query_d{dimension}_{bucket}_q{query_id}.txt"

                with open(query_file, "w") as f:
                    f.write(f"Dimension: {dimension}\n")
                    f.write(f"Bucket: {bucket}\n")
                    f.write(f"Query ID: {query_id}\n\n")

                    for i, (lo, hi) in enumerate(qbox):
                        f.write(f"Dim {i+1}: {lo} {hi}\n")


                temp_file=f"dataset_d{dimension}.txt"


                write_temp_dataset(
                    temp_file,
                    n,
                    dimension,
                    coords,
                    colors,
                    qbox
                )
                

                print()
                print("Query Box")
                print(qbox)
                print()
                for solver in SOLVERS:
                    print(f"Running {solver['name']}")
                    runtime,similarity,status=run_solver(
                        solver,
                        temp_file,
                        constraint_file
                    )
                    append_csv(
                        dimension,
                        bucket,
                        query_id,
                        input_points,
                        qbox,
                        target_box,
                        target_sim,
                        constraint_file,
                        solver["name"],
                        runtime,
                        similarity,
                        status
                    )
                    print(f"Runtime={runtime} "
                        f"Similarity={similarity} "
                        f"Status={status}"
                    )
                query_id+=1

    print()
    print("BENCHMARK FINISHED")

if __name__=="__main__":
    main()