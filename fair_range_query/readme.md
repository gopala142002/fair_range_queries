# Fairness-Aware Range Query Framework

This repository contains implementations for the **Fairness-Aware Range Query Problem**, where the objective is to compute a range query result that satisfies one or more fairness constraints while maximizing its similarity to the original query.

The repository consists of two main components:

* **`Combined-upload/`** — Implements multiple algorithms for fair range query processing, including KD-Tree, Range Tree, R-Tree, R*-Tree, X-Tree, BFS, Brute Force, and Coverage Trees under Difference, Ratio, and Coverage fairness constraints.

* **`mult-predicate/`** — Contains Integer Linear Programming (ILP) solvers for computing optimal fair range queries using Dinkelbach's Algorithm, Binary Search, Coordinate Compression, Dynamic Gridding, Warm Starts and BFS approach.

Together, these components provide both algorithmic and optimization-based approaches for evaluating fairness-aware range queries under multiple fairness conditions.

---

# Repository Structure

```text
.
├── Combined-upload/      # Algorithms and experiment framework
├── mult-predicate/       # ILP solvers
└── README.md             # This file
```

---

# How to Run

Each component has its own dependencies, input formats, and execution instructions.

* For **`single-predicate/`**, refer to **`single-predicate/README.md`**.
* For **`multi-predicate/`**, refer to **`multi-predicate/README.md`**.

Please follow the appropriate README for the component you wish to use.