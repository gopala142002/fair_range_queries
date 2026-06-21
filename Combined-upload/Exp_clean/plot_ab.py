import sys
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ── CLI ───────────────────────────────────────────────────────────────────────
if len(sys.argv) != 3:
    print("Usage: python plot_ab.py <csv_file> <metric>")
    print("  metric: S or Q")
    sys.exit(1)

csv_path = sys.argv[1]
metric   = sys.argv[2].upper()

if metric not in ('S', 'Q'):
    print(f"Error: metric must be S or Q (got '{metric}')")
    sys.exit(1)

if not os.path.isfile(csv_path):
    print(f"Error: file not found: {csv_path}")
    sys.exit(1)

# ── Load & label ──────────────────────────────────────────────────────────────
df = pd.read_csv(csv_path)

mode_map = {'range_opt': 'Range', 'kd_opt': 'KD', 'rtree_opt': 'R', 'rstar_opt': 'R*'}

def get_algo_label(row):
    if row['algorithm'] == 'brute': return 'Brute'
    if row['algorithm'] == 'bfs':   return 'BFS'
    if row['algorithm'] == 'trees':
        return mode_map.get(row['mode'], row['mode'])
    return None

df['algo_label'] = df.apply(get_algo_label, axis=1)
df = df[df['algo_label'].notna()]
df['ab_label'] = df.apply(lambda r: f'α={r["alpha"]} β={r["beta"]}', axis=1)

AB_ORDER  = [f'α={a} β={b}' for a, b in [(0.2,0.8),(0.4,0.6),(0.6,0.4),(0.8,0.2)]]
AB_COLORS = {'α=0.2 β=0.8': '#4C9BE8', 'α=0.4 β=0.6': '#F4A261',
             'α=0.6 β=0.4': '#2EC4B6', 'α=0.8 β=0.2': '#E07BE0'}
ALGOS     = ['Brute', 'BFS', 'Range', 'KD', 'R', 'R*']

FIG_BG   = 'white'
AX_BG    = 'white'
GRID_COL = '#BBBBBB'

def style_ax(ax, title, ylabel, log=False):
    ax.set_facecolor(AX_BG)
    ax.set_title(title, color='black', fontsize=22, pad=10)
    ax.set_ylabel(ylabel, color='black', fontsize=20)
    ax.tick_params(colors='black')
    ax.xaxis.set_tick_params(labelcolor='black', labelsize=18)
    ax.yaxis.set_tick_params(labelcolor='black', labelsize=18)
    for spine in ax.spines.values():
        spine.set_edgecolor(GRID_COL)
    ax.grid(axis='y', color=GRID_COL, linestyle='--', linewidth=0.5, zorder=0)
    if log: ax.set_yscale('log')

def bar_label(val):
    if val < 1: return f'{val:.3f}'
    elif val < 1000: return f'{val:.1f}'
    else: return f'{val/1000:.1f}k'

if metric == 'S':
    means = df.groupby('ab_label')['similarity'].mean().reindex(AB_ORDER)
    fig, ax = plt.subplots(figsize=(12, 7))
    fig.patch.set_facecolor(FIG_BG)
    style_ax(ax, 'Average Similarity by α/β Combination', 'Average Similarity')
    ax.set_ylim(0, 1)
    x = np.arange(len(AB_ORDER))
    bars = ax.bar(x, means.values, width=0.5,
                  color=[AB_COLORS[ab] for ab in AB_ORDER],
                  edgecolor='white', linewidth=0.6, zorder=3)
    for bar, val in zip(bars, means.values):
        ax.text(bar.get_x()+bar.get_width()/2, val*1.01, f'{val:.3f}',
                ha='center', va='bottom', fontsize=16, color='black')
    ax.set_xticks(x)
    ax.set_xticklabels(AB_ORDER, color='black', fontsize=18)

elif metric == 'Q':
    means = (df.groupby(['algo_label','ab_label'])['avg_query_time_ms']
               .mean().unstack('ab_label')
               .reindex(ALGOS).reindex(columns=AB_ORDER))
    bar_width_ = 0.18
    group_width_ = len(AB_ORDER)*bar_width_ + 0.12
    x_centers_ = np.arange(len(ALGOS)) * group_width_
    fig, ax = plt.subplots(figsize=(18, 7))
    fig.patch.set_facecolor(FIG_BG)
    style_ax(ax, 'Average Query Time by Algorithm and α/β Combination',
             'Average Query Time (ms, log scale)', log=True)
    for i, ab in enumerate(AB_ORDER):
        offsets = x_centers_ + (i-1.5)*bar_width_ + bar_width_/2
        vals = means[ab].values
        bars = ax.bar(offsets, vals, width=bar_width_, color=AB_COLORS[ab],
                      label=ab, edgecolor='white', linewidth=0.6, zorder=3)
        for bar, val in zip(bars, vals):
            if pd.isna(val): continue
            ax.text(bar.get_x()+bar.get_width()/2, val*1.15,
                    bar_label(val), ha='center', va='bottom',
                    fontsize=14, color='black', rotation=45)
    ax.set_xticks(x_centers_)
    ax.set_xticklabels(ALGOS, color='black', fontsize=20)

# ── Save ──────────────────────────────────────────────────────────────────────
base     = os.path.splitext(os.path.basename(csv_path))[0]
out_name = f"{base}_{metric}.png"
out_path = os.path.join(os.path.dirname(os.path.abspath(csv_path)), out_name)

plt.tight_layout(pad=2.0)
plt.savefig(out_path, dpi=150, bbox_inches='tight', facecolor='white')
print(f"Saved: {out_path}")
