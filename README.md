# Fair Range Query — Multi-Color Extension

Extension of Shetiya et al. (ICDE 2022) fairness-aware range queries to support multiple demographic groups.

## What it does

Given a dataset with multiple color groups and a user range query, finds the most similar range satisfying:
- **Fairness** — max color count - min color count ≤ epsilon
- **Coverage** (optional) — each color must appear at least a required number of times
- **Similarity** — measured using Tversky index (generalises Jaccard)

## Folder Structure

```
Multicolor_Trees/
├── src/
│   ├── common/     # Shared code, CSV parsing, Tversky index
│   ├── trees/      # KD-Tree, Range Tree, R-Tree, R*-Tree, X-Tree
│   ├── query/      # Optimized and coverage query variants
│   ├── bfs/        # BFS-based fair range search
│   ├── brute/      # Brute force O(n²) baseline
│   └── main/       # Entry points
├── data/           # Datasets
├── tools/          # Dataset converter
└── Makefile
```

## Build

```bash
make
```

Or compile a single program:

```bash
g++ -O2 -std=c++17 -Isrc/common -Isrc/trees src/common/common.cpp src/common/coverage.cpp src/trees/range_tree.cpp src/main/main_range.cpp -o bin/main_range
```

## Run

```bash
bin/main_range data/1000_3 3
```

Input format:
```
epsilon start end alpha beta
```

For coverage programs:
```
epsilon start end alpha beta min_Color1 min_Color2 ...
```

## Programs

| Program | Description |
|---|---|
| `main_brute_fair` | Brute force fairness only |
| `main_brute_fair_cov` | Brute force fairness + coverage |
| `main_kd` / `main_kd_opt` / `main_kd_cov` | KD-Tree variants |
| `main_range` / `main_range_opt` / `main_range_cov` | Range Tree variants |
| `main_rtree` / `main_rtree_opt` / `main_rtree_cov` | R-Tree variants |
| `main_rstar` / `main_rstar_opt` / `main_rstar_cov` | R*-Tree variants |
| `main_xtree` / `main_xtree_opt` / `main_xtree_cov` | X-Tree variants |
| `main_bfs_fair` | BFS fairness only |
| `main_bfs_fair_cov` | BFS fairness + coverage |

## Dataset Format

```
index,color,Red-Blue,Red-Green,Blue-Green
1,Green,0,-1,-1
2,Blue,-1,-1,0
```

Convert your own dataset:
```bash
bin/convert_dataset input_file output_file
```

## Reference

Shetiya et al., "Fairness-Aware Range Queries for Selecting Unbiased Data", ICDE 2022.
