====================================================================
  Experiment Plotting Scripts — README
  Fair Range Query with Multiclass Fairness Constraints
====================================================================

REQUIREMENTS
------------
Python 3.8+  with the following packages:
    pip install pandas matplotlib numpy

--------------------------------------------------------------------
FOLDER STRUCTURE
--------------------------------------------------------------------

Exp_clean/
├── plot_box.py            Ablation box plots (similarity or query time)
├── plot_ab.py             Alpha-beta sensitivity bar charts
├── plot_sensitivity.py    Parameter sensitivity line plots (eps, delta, theta)
├── plot_time_sim.py       Time vs similarity scatter plots by range bucket
├── make_table.py          Generate LaTeX tables from CSV results
├── legend/
│   └── generate_legends.py  Standalone legend image generator
│
├── 40000_5.csv            Synthetic dataset results (40k points, 5 groups)
├── 40000_5_AB.csv         Alpha-beta results for 40k synthetic
├── 32k.csv                Synthetic dataset results (32k points)
├── 32K_AB.csv             Alpha-beta results for 32k synthetic
├── F1.csv                 F1 lap telemetry dataset results
├── F1_AB.csv              Alpha-beta results for F1 dataset
├── F1_varying_cov_weighted.csv     Coverage-weighted results for F1
├── F1_varying_cov_weighted_AB.csv  Alpha-beta for coverage-weighted F1
├── filtered_DRC.csv       DRC fairness combination filtered results
│
├── Ablation_box/          Output folder for plot_box.py  (created on run)
├── AB_Q/                  Output folder for plot_ab.py query time  (created on run)
├── AB_S/                  Output folder for plot_ab.py similarity  (created on run)
├── time_sim/              Output folder for plot_time_sim.py  (created on run)
├── vary_EDT/              Output folder for plot_sensitivity.py  (created on run)
└── Fairness/              Output folder for make_table.py  (created on run)

--------------------------------------------------------------------
SCRIPT USAGE
--------------------------------------------------------------------

1. plot_box.py — Ablation box plots
   Generates box plots comparing algorithms across range buckets.
   Metrics: S = similarity, Q = query time (log scale)

   Usage:
       python plot_box.py <csv_file> <metrics_code>

   metrics_code options:
       S    — similarity only
       Q    — query time only
       SQ   — both side by side

   Examples:
       python plot_box.py 40000_5.csv S
       python plot_box.py F1.csv SQ
       python plot_box.py 32k.csv Q

   Output: Ablation_box/<dataset>_S.png  or  _Q.png  or  _SQ.png


2. plot_ab.py — Alpha-beta sensitivity bar charts
   Compares algorithm performance across alpha-beta Tversky configurations.
   Metric: S = similarity, Q = query time

   Usage:
       python plot_ab.py <csv_file> <metric>

   Examples:
       python plot_ab.py 40000_5_AB.csv S
       python plot_ab.py F1_AB.csv Q
       python plot_ab.py 32K_AB.csv S

   Output: AB_S/<dataset>_S.png  or  AB_Q/<dataset>_Q.png


3. plot_sensitivity.py — Parameter sensitivity line plots
   Plots similarity and query time while varying epsilon, delta, or theta.

   Usage:
       python plot_sensitivity.py <csv_file>

   Examples:
       python plot_sensitivity.py filtered_DRC.csv
       python plot_sensitivity.py F1_varying_cov_weighted.csv

   Output (in vary_EDT/):
       vary_theta.png
       vary_epsilon.png
       vary_delta.png


4. plot_time_sim.py — Time vs similarity scatter by range bucket
   Shows trade-off between query time and similarity for small/medium/large ranges.

   Usage:
       python plot_time_sim.py <csv_file> <buckets>

   buckets options (any combination of S, M, L):
       S    — small ranges only
       M    — medium ranges only
       L    — large ranges only
       SML  — all three buckets

   Examples:
       python plot_time_sim.py F1.csv SML
       python plot_time_sim.py 40000_5.csv SM
       python plot_time_sim.py 32k.csv L

   Output: time_sim/<dataset>_SML.png  (or whichever buckets selected)


5. make_table.py — Generate LaTeX tables
   Produces a LaTeX table summarising results per algorithm and range bucket.

   Usage:
       python make_table.py <csv_file> [--preview]

   --preview  prints the table to terminal instead of saving to file

   Examples:
       python make_table.py F1.csv
       python make_table.py 40000_5.csv --preview
       python make_table.py F1_varying_cov_weighted.csv

   Output: Fairness/<dataset>_table.tex


6. legend/generate_legends.py — Generate standalone legend images
   Creates reusable legend PNG files for all plot types.
   Run once — output is placed in the legend/ folder.

   Usage:
       python legend/generate_legends.py

   Output (in legend/):
       legend_algos.png       — algorithm legend (Brute, BFS, Range, KD, R, R*)
       legend_buckets.png     — range bucket legend (Small, Medium, Large)
       legend_ab.png          — alpha-beta legend
       legend_convergence.png — convergence legend

--------------------------------------------------------------------
TYPICAL WORKFLOW
--------------------------------------------------------------------

Step 1 — Generate legends once:
    python legend/generate_legends.py

Step 2 — Generate all box plots:
    python plot_box.py 40000_5.csv SQ
    python plot_box.py 32k.csv SQ
    python plot_box.py F1.csv SQ
    python plot_box.py F1_varying_cov_weighted.csv SQ

Step 3 — Generate alpha-beta plots:
    python plot_ab.py 40000_5_AB.csv S
    python plot_ab.py 40000_5_AB.csv Q
    python plot_ab.py F1_AB.csv S
    python plot_ab.py F1_AB.csv Q

Step 4 — Generate sensitivity plots:
    python plot_sensitivity.py filtered_DRC.csv
    python plot_sensitivity.py F1_varying_cov_weighted.csv

Step 5 — Generate time-similarity plots:
    python plot_time_sim.py F1.csv SML
    python plot_time_sim.py 40000_5.csv SML
    python plot_time_sim.py 32k.csv SML

Step 6 — Generate LaTeX tables:
    python make_table.py 40000_5.csv
    python make_table.py 32k.csv
    python make_table.py F1.csv
    python make_table.py F1_varying_cov_weighted.csv

--------------------------------------------------------------------
NOTES
--------------------------------------------------------------------
- All scripts save output to the same directory as the input CSV
  (or to the dedicated subfolders listed above).
- Output folders (Ablation_box/, AB_Q/, etc.) are created automatically
  if they do not exist.
- All plots use white background, 300 DPI, no internal legends
  (use the standalone legend files from generate_legends.py instead).
====================================================================
