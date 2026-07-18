# Fairness-Aware Range Query: ILP Solvers

This directory contains the Integer Linear Programming (ILP) solvers designed to address the **Fairness-Aware Range Query Problem**. 

The goal of these solvers is to compute an output bounding box $O$ that maximizes the **Jaccard Similarity** (or a fractional equivalent) with an initial multidimensional query box $I$, while strictly adhering to multiple group fairness constraints (such as color/group coverage, ratio bounds, and difference bounds). 

Since Jaccard Similarity is a non-linear fractional objective, the problem is solved iteratively using combinations of **Dinkelbach's Algorithm** and **Binary Search**, optimized with Coordinate Compression, Dynamic Gridding, and bounded exact search spaces.

---

## Datasets Used

The solvers are evaluated against a variety of real-world datasets spanning different dimensions and cardinalities:
<!-- *   **NYC**: New York City spatial dataset (`NYC.csv`, `NYC_30k_5colors.txt`).
*   **Gaia**: Astronomical dataset featuring star coordinates (`Gaia_30k_5colors.txt`, `dataGaia.csv`).
*   **ACS (American Community Survey)**: Demographic dataset (`small_acs_3d_points.txt`).
*   **Realtor**: Real estate listing coordinates (`realtor.txt`, `realtor_10000.txt`). -->
*   **Synthetic**: Synthetic dataset (`synthetic_4d.cav`)
*   **Live L2**: Custom/System dataset (`live_l2_5d.txt`, `live_l2_shadow.txt`).

---

## Input Data Format (`points.txt`)

The primary input coordinate and query boundary data must be formatted strictly in a structured text file (typically `points.txt` or similar). 

> **Important Note on Target Dimensions ($t$):** The value for $t$ (target non-spatial attributes) **must be 0**. The mathematical formulation bounds only the $d$ spatial dimensions. Target attributes are not supported in the Big-M bounding logic.

### Format Structure:
```text
n_points         # Total number of points (n)
d t              # Number of spatial dims (d), target dims (t) MUST BE 0
x1 x2 ... xd     # Point 1 coords (d spatial dimensions)
y1 y2 ... yd     # Point 2 coords
...              # (Total n lines for points)
c1 c2 ... cn     # A single line containing exactly n space-separated color/group IDs
min_1 max_1      # Dim 1 Initial Query Box Bounds (min max)
min_2 max_2      # Dim 2 Initial Query Box Bounds (min max)
...              # (Total d lines for query bounds)
```

---

## Constraints Format (`constraints.txt`)

The fairness rules are passed externally via a text file (typically `constraints.txt`) and parsed by `parse_constraints.py`. The file defines pairwise differences, ratios, and coverage metrics across $N$ unique colors.

### Format Structure:
You must provide explicit block headers followed by $N \times N$ matrices or $1 \times N$ arrays. Use `NaN` to indicate that a specific pairwise bound is unrestricted.

```text
Weights of color:
1.0 1.0 1.0

Coverage:
206 1246 785

Difference:
NaN NaN NaN
NaN NaN NaN
25 NaN NaN

Ratio:
NaN NaN NaN
6.04 NaN 1.58
3.81 NaN NaN
```
*(In this example, Color 3 is constrained to have at most 25 more points than Color 1. Color 2 is bounded by a ratio of 6.04 against Color 1, etc.)*

---

## Execution & Usage

The solvers can be run directly from the command line using a Python 3 environment with `gurobipy` installed.

### Available Solvers:
1. **Dynamic Grid + Dinkelbach (Two-Pass Approx-Exact):**
   `python solve_ilp_dynamic_grid_dinkelbach_warm_start_bounded_optimized_copy.py`
2. **Dynamic Grid + Binary Search (Two-Pass Approx-Exact):**
   `python solve_ilp_dynamic_grid_binary_search_warm_start_bounded.py`
3. **Exact Dinkelbach (Exhaustive Space):**
   `python solve_ilp_Dinkleback_gamma_bounded.py`
4. **Exact Binary Search (Exhaustive Space):**
   `python solve_ilp_binary_search_gamma_bounded.py`

### Running the Solvers:
By default, executing a solver without arguments will look for `points.txt` and `constraints.txt` in the current working directory. You can optionally provide them as positional arguments:

```bash
# Example: Default execution
python solve_ilp_dynamic_grid_dinkelbach_warm_start_bounded_optimized_copy.py

# Example: Specifying custom datasets and constraints
python solve_ilp_binary_search_gamma_bounded.py data/NYC_30k_5colors.txt constraints_nyc.txt
```

### Standard Output:
The terminal will dump the parsed inputs, the current iterations of fractional programming feasibility, the overall execution time, the exact boundaries of the final optimal bounding box, and its verified Jaccard similarity.

## ⚙️ Parameters
Inside the `__main__` execution block of the solvers, you can tweak:
*   `alpha`, `beta`: (Default 1.0). Controls the fraction penalty for points strictly inside the query box ($S_{io}$) or strictly inside the output box ($S_{oi}$).
*   `delta_base`: (Default 2). Controls the logarithmic expansion sparsity of the Dynamic Grid.
*   `epsilon`: (Default 0.0). Global fallback tolerance if pairwise bounds aren't given.
