import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import numpy as np

# ── CLI ───────────────────────────────────────────────────────────────────────
if len(sys.argv) != 2:
    print("Usage: python plot_sensitivity.py <csv_file>")
    sys.exit(1)

CSV_PATH = sys.argv[1]
if not os.path.isfile(CSV_PATH):
    print(f"Error: file not found: {CSV_PATH}")
    sys.exit(1)

df = pd.read_csv(CSV_PATH)

mode_map = {'range_opt': 'Range', 'kd_opt': 'KD', 'rtree_opt': 'R', 'rstar_opt': 'R*'}

def get_algo(row):
    if row['algorithm'] == 'brute': return 'Brute'
    if row['algorithm'] == 'bfs':   return 'BFS'
    if row['algorithm'] == 'trees': return mode_map.get(row['mode'], row['mode'])
    return None

df['algo_label'] = df.apply(get_algo, axis=1)
df = df[df['algo_label'].notna()]

BASE_EPS   = 10
BASE_DELTA = 0
BASE_THETA = 5

FIG_BG   = 'white'
AX_BG    = 'white'
GRID_COL = '#BBBBBB'

ALGOS = ['Brute', 'BFS', 'Range', 'KD', 'R', 'R*']
ALGO_COLORS  = {'Brute': '#E07BE0', 'BFS': '#F4A261', 'Range': '#4C9BE8',
                'KD': '#2EC4B6', 'R': '#E63946', 'R*': '#A8DADC'}
ALGO_MARKERS = {'Brute': 'o', 'BFS': 's', 'Range': '^',
                'KD': 'D', 'R': 'v', 'R*': 'P'}

def style_ax(ax, title, ylabel, xlabel, log=False):
    ax.set_facecolor(AX_BG)
    ax.set_title(title, color='black', fontsize=22, pad=8)
    ax.set_ylabel(ylabel, color='black', fontsize=20)
    ax.set_xlabel(xlabel, color='black', fontsize=20)
    ax.tick_params(colors='black')
    ax.xaxis.set_tick_params(labelcolor='black', labelsize=18)
    ax.yaxis.set_tick_params(labelcolor='black', labelsize=18)
    for spine in ax.spines.values():
        spine.set_edgecolor(GRID_COL)
    ax.grid(color=GRID_COL, linestyle='--', linewidth=0.5, zorder=0)
    if log: ax.set_yscale('log')

def make_plot(df_sub, vary_col, x_vals, x_label, fixed_label, out_path):
    fig, (ax_sim, ax_q) = plt.subplots(2, 1, figsize=(16, 12))
    fig.patch.set_facecolor(FIG_BG)
    fig.suptitle(f'Sensitivity to {x_label}  |  {fixed_label}',
                 color='black', fontsize=24, y=0.98)

    agg = (df_sub.groupby(['algo_label', vary_col])[['avg_query_time_ms', 'similarity']]
                 .mean().reset_index())

    sim_mean = agg.groupby(vary_col)['similarity'].mean().reindex(x_vals)
    style_ax(ax_sim, 'Average Similarity', 'Similarity', x_label)
    ax_sim.set_ylim(0, 1)
    ax_sim.set_xticks(x_vals)
    ax_sim.plot(x_vals, sim_mean.values, color='#4C9BE8',
                marker='o', linewidth=2, markersize=8, zorder=3)
    for x, y in zip(x_vals, sim_mean.values):
        if not np.isnan(y):
            ax_sim.annotate(f'{y:.3f}', (x, y),
                            textcoords='offset points', xytext=(0, 10),
                            ha='center', color='black', fontsize=16)

    style_ax(ax_q, 'Average Query Time', 'Query Time (ms, log scale)', x_label, log=True)
    ax_q.set_xticks(x_vals)

    for algo in ALGOS:
        sub = agg[agg['algo_label']==algo].set_index(vary_col)['avg_query_time_ms'].reindex(x_vals)
        if sub.isna().all(): continue
        ax_q.plot(x_vals, sub.values, color=ALGO_COLORS[algo],
                  marker=ALGO_MARKERS[algo], linewidth=2, markersize=8,
                  label=algo, zorder=3)

    plt.tight_layout(rect=[0, 0, 1, 0.96])
    plt.savefig(out_path, dpi=150, bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"Saved: {out_path}")

OUT_DIR = os.path.dirname(os.path.abspath(CSV_PATH))

theta_vals = sorted(df['coverage_percent'].unique())
eps_vals   = sorted(df['epsilon'].unique())
delta_vals = sorted(df['delta'].unique())

make_plot(df[(df['epsilon']==BASE_EPS) & (df['delta']==BASE_DELTA)],
          'coverage_percent', theta_vals, 'Coverage % (θ)',
          f'ε={BASE_EPS}, δ={BASE_DELTA}',
          os.path.join(OUT_DIR, 'vary_theta.png'))

make_plot(df[(df['delta']==BASE_DELTA) & (df['coverage_percent']==BASE_THETA)],
          'epsilon', eps_vals, 'Epsilon (ε)',
          f'δ={BASE_DELTA}, θ={BASE_THETA}%',
          os.path.join(OUT_DIR, 'vary_epsilon.png'))

make_plot(df[(df['epsilon']==BASE_EPS) & (df['coverage_percent']==BASE_THETA)],
          'delta', delta_vals, 'Delta (δ)',
          f'ε={BASE_EPS}, θ={BASE_THETA}%',
          os.path.join(OUT_DIR, 'vary_delta.png'))
