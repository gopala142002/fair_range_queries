## A Unified Framework for Fair Range Queries under Multiple Fairness Conditions

## 1. CONFIGURATION

Constraints are entered interactively at runtime via console prompts.
You will be asked to specify Difference constraints (epsilon, weights) and Ratio constraints (delta, weights).

**Example interaction:**

```
  Enter number of Difference constraints: 1
    Constraint 1 - Enter ColorA ColorB weightA weightB Epsilon: Florida California 1.0 1.0 50.0
  Enter number of Ratio constraints: 0
```

> **NOTE:** Ensure the color/state names match EXACTLY what is in the dataset (case-sensitive). State names use underscores for spaces (e.g., `New_York`, `North_Carolina`, `Puerto_Rico`).

## 2. COMPILATION

Whenever you change the source code, recompile by running:

```bash
make
```

To rebuild from scratch:

```bash
make clean
make
```

This creates the following executables inside the `bin/` directory:

- `main_trees_unified.exe`    (Tree-based search algorithms)
- `main_bfs_unified.exe`      (BFS search)
- `main_brute_unified.exe`    (Brute force search -- standalone, run separately)
- `main_coverage_unified.exe` (Coverage tree search)

## 3. AVAILABLE DATASETS

The project uses the following primary datasets for experimentation. Ensure these files are located in your `data/` directory.

1. `data/40000_5`

   - **Type:** Synthetic dataset (40,000 rows)
   - **Colors:** 5 (Numeric indices: 0, 1, 2, 3, 4)
2. `data/f1_processed.csv`

   - **Type:** Real-world Formula 1 dataset
   - **Colors:** 5 (HARD, MEDIUM, SOFT, INTERMEDIATE, WET)
3. `data/live_l2_1D.csv`

   - **Type:** Live L2 Crypto Orderbook dataset
   - **Colors:** 3 (ETHUSDT, BTCUSDT, SOLUSDT)

## 4. HOW TO RUN

You can use the built-in `make run_*` commands or run the binaries directly.

> **IMPORTANT:** The standard tree search algorithms in **Section A** evaluate Difference and Ratio constraints but do **NOT** include Coverage constraints. If you want to enforce Difference, Ratio, AND Coverage constraints all at once, you must use the Coverage Trees in **Section D**.

### A. TREE SEARCH ALGORITHMS

Runs the core multidimensional spatial trees.
Available modes: `kd`, `kd_opt`, `range`, `range_opt`, `rtree`, `rtree_opt`, `rstar`, `rstar_opt`, `xtree`, `xtree_opt`

**Using Make (Example with 40k synthetic dataset):**

```bash
make run_trees DATASET="data/40000_5" COLORS=5 TREE_MODE="kd_opt"
```

**Direct execution:**

```bash
.\bin\main_trees_unified.exe data/40000_5 5 kd_opt
```

### B. BFS SEARCH

Runs a graph-search Breadth-First-Search algorithm.
Available modes: `fair`, `fair_cov`

**Using Make (Example with F1 dataset):**

```bash
make run_bfs DATASET="data/f1_processed.csv" COLORS=5 BFS_MODE="fair_cov"
```

**Direct execution:**

```bash
.\bin\main_bfs_unified.exe data/f1_processed.csv 5 fair_cov
```

### C. BRUTE FORCE SEARCH (standalone)

Tests every possible interval. Run separately to verify correctness against trees.
Available modes: `base`, `fair`, `fair_cov`

**Using Make (Example with Live Crypto dataset):**

```bash
make run_brute DATASET="data/live_l2_1D.csv" COLORS=3 BRUTE_MODE="fair"
```

**Direct execution:**

```bash
.\bin\main_brute_unified.exe data/live_l2_1D.csv 3 fair
```

> **NOTE:** Brute force is O(n^2) and is NOT bundled with tree/BFS/coverage executables. Use it independently on small ranges for correctness verification.

### D. COVERAGE TREES

Runs spatial trees that guarantee a minimum number of colors using required thresholds.
Available modes: `kd`, `kd_opt`, `range`, `range_opt`, `rtree`, `rtree_opt`, `rstar`, `rstar_opt`, `xtree`, `xtree_opt`

**Using Make (Example with 40k synthetic dataset):**

```bash
make run_coverage DATASET="data/40000_5" COLORS=5 COV_MODE="kd"
```

**Direct execution:**

```bash
.\bin\main_coverage_unified.exe data/40000_5 5 kd
```

## 5. EXECUTING QUERIES

Once any of the above executables launch, you interact with them via console.

**Step 1:** Enter fairness constraints when prompted (see Section 1).

> Make sure to use the exact color string for your dataset (e.g., "HARD", "ETHUSDT", or "1").

**Step 2:** Enter query parameters when prompted.

**For standard modes (trees, BFS fair, brute base/fair):**

```
  Prompt: Enter start end alpha beta (-1 to exit):
  Example (equal false-pos/neg weight):
    1 1000 0.5 0.5
```

> **If you ran a Coverage mode**, the prompt expects additional inputs for the minimum allowed items for each color ON THE SAME LINE.

```
  Prompt: Enter start end alpha beta min_Color1 min_Color2 ... (-1 to exit):
  Example (for 5 colors, appending 5 integer requirements):
    1 1000 0.5 0.5 10 10 10 10 10
```

## 6. RECOMMENDED TEST RANGES

Depending on the dataset, the range parameters (start, end) refer to either row indices or timestamps. Reference the Python scripts (like `run_all_combinations_f1.py`) for specific timestamps used in large experiments.

**For index-based synthetic data like `40000_5`:**

- Small test:   `1 1000 0.5 0.5`
- Medium test:  `1 10000 0.5 0.5`
- Large test:   `10000 30000 0.5 0.5`

**Suggested constraint for testing (Synthetic dataset):**

- 1 Difference constraint: `0 1 50.0`
- 0 Ratio constraints

## 8. AUTOMATED PYTHON EXPERIMENT SCRIPTS

The `scratch/` directory contains automated Python scripts that run large batches of experiments. These scripts automatically launch the C++ executables via subprocess, supply queries and combinations, and save the timing/results to CSV files.

To run any of these, execute them using Python from the project root. For example:

```bash
python scratch/run_all_combinations_f1.py
```

### Modifiable Parameters

At the top of each script (inside the `main()` function), you can modify several configuration variables to customize the experiments:

- `dataset` & `num_colors`: Which dataset file to load and the number of colors it contains.
- `alpha` & `beta`: The Tversky index weights for false positives and false negatives.
- `epsilons`: A list of difference constraint values (e.g., `[0.0, 2.0, 10.0, 100.0]`) to iterate over.
- `deltas`: A list of ratio constraint values (e.g., `[0.0, 1.0, 5.0]`) to iterate over.
- `weightA` & `weightB`: The weights used for Ratio and Difference equations.
- `tree_modes`: A list of algorithm modes to test (e.g., `["kd_opt", "rtree_opt"]`).
- `combinations`: A list of boolean tuples `(has_diff, has_ratio, has_cov)` determining which types of fairness constraints are actively tested.
- `widths`: Defines the size of the query intervals tested (e.g., small, medium, large buckets relative to dataset size).

### A. Standard Combination Scripts

Tests a wide variety of fairness constraint combinations (Difference only, Ratio only, Difference + Ratio, etc.) across tree search, BFS, and Brute Force methods.

- `scratch/run_all_combinations_f1.py`
- `scratch/run_all_combinations_livel2.py`
- `scratch/run_all_combinations_40k_opt.py`

### B. Alpha & Beta Combinations Scripts

Specifically designed to test how varying the `alpha` and `beta` parameters (Tversky index) affects query times and similarity scores.

- `scratch/run_alpha_beta_combinations_40000_5.py`
- `scratch/run_alpha_beta_combinations_f1.py`
- `scratch/run_alpha_beta_combinations_livel2.py`

### C. Coverage Combination Scripts

Focuses primarily on Coverage constraints, evaluating how different coverage percentage thresholds (e.g., 2%, 5%) impact the algorithms.

- `scratch/run_coverage_40k_opt.py`
- `scratch/run_coverage_f1.py`
- `scratch/run_coverage_livel2.py`

### Tracking Execution Progress

Because these scripts evaluate thousands of combinations, execution can be lengthy. Progress is tracked incrementally in the console.
Additionally, scripts designated with `_progress` (like `scratch/run_all_combinations_livel2_progress.py`) are designed to output periodic updates and incrementally append results to their respective output files (e.g. `combinations_result_livel2_progress.csv`), allowing you to monitor results in real time.

## 9. PLOTTING RESULTS
The `Exp_clean/` directory contains Python scripts to generate graphs from the CSV results produced by the experiment scripts. All plots will be saved as PNG images in the same directory as the input CSV file.

Ensure you have the required dependencies:
```bash
pip install pandas matplotlib numpy
```

### Available Plotting Scripts
- **plot_ab.py**: Generates A/B comparison plots for specific metrics.
  `python Exp_clean/plot_ab.py <csv_file> <metric>` *(metric: S for Similarity, Q for Query Time)*
- **plot_box.py**: Generates box plots for similarity and query time distribution across buckets.
  `python Exp_clean/plot_box.py <csv_file> <metrics_code>` *(metrics_code: S, Q, or SQ)*
- **plot_sensitivity.py**: Generates sensitivity analysis plots varying epsilon, delta, and theta.
  `python Exp_clean/plot_sensitivity.py <csv_file>`
- **plot_time_sim.py**: Plots the progression of similarity vs time for specific bucket sizes.
  `python Exp_clean/plot_time_sim.py <csv_file> <buckets>` *(buckets: any combination of S, M, L for Small, Medium, Large)*
