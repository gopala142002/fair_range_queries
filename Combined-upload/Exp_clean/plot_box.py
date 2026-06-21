import sys
import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# ── CLI ───────────────────────────────────────────────────────────────────────
if len(sys.argv) != 3:
    print("Usage: python plot_box.py <csv_file> <metrics_code>")
    print("  metrics_code: any ordered combination of S, Q")
    print("  S = similarity | Q = query time (log scale)")
    sys.exit(1)

csv_path    = sys.argv[1]
metric_code = sys.argv[2].upper()

valid_chars = set("SQ")
if not all(c in valid_chars for c in metric_code) or len(metric_code) == 0:
    print(f"Error: metrics_code must be S, Q, or SQ (got '{metric_code}')")
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

# ── Config ────────────────────────────────────────────────────────────────────
ALGOS        = ['Brute', 'BFS', 'Range', 'KD', 'R', 'R*']
BUCKET_ORDER = ['small', 'medium', 'large']
BUCKET_COLORS = {'small': '#4C9BE8', 'medium': '#F4A261', 'large': '#2EC4B6'}
BUCKET_LABELS = {'small': 'Small', 'medium': 'Medium', 'large': 'Large'}
ALGO_COLORS  = {'Brute': '#E07BE0', 'BFS': '#F4A261', 'Range': '#4C9BE8',
                'KD': '#2EC4B6', 'R': '#E63946', 'R*': '#A8DADC'}

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
    if log:
        ax.set_yscale('log')

def style_box(bp, color):
    for element in ['boxes', 'whiskers', 'caps']:
        for item in bp[element]:
            item.set_color(color)
            item.set_linewidth(1.8)
    for median in bp['medians']:
        median.set_color('black')
        median.set_linewidth(2)
    for flier in bp['fliers']:
        flier.set(marker='o', markerfacecolor=color,
                  markeredgecolor=color, markersize=3, alpha=0.5)

# ── Figure ────────────────────────────────────────────────────────────────────
n_plots   = len(metric_code)
fig, axes = plt.subplots(n_plots, 1, figsize=(18, 8 * n_plots))
if n_plots == 1:
    axes = [axes]
fig.patch.set_facecolor(FIG_BG)

n_algos     = len(ALGOS)
box_width   = 0.12
group_gap   = 0.5
group_width = n_algos * box_width + group_gap
group_centers = np.arange(3) * group_width

for ax, code in zip(axes, metric_code):
    if code == 'S':
        style_ax(ax, 'Similarity Distribution by Range Bucket', 'Similarity')
        ax.set_ylim(0, 1)
        for i, bucket in enumerate(BUCKET_ORDER):
            data = df[df['range_bucket'] == bucket]['similarity'].dropna().values
            bp = ax.boxplot(data, positions=[group_centers[i]], widths=0.4,
                            patch_artist=False, manage_ticks=False)
            style_box(bp, BUCKET_COLORS[bucket])
        ax.set_xticks(group_centers)
        ax.set_xticklabels([BUCKET_LABELS[b] for b in BUCKET_ORDER],
                           color='black', fontsize=20)

    elif code == 'Q':
        style_ax(ax, 'Query Time Distribution by Range Bucket and Algorithm',
                 'Query Time (ms, log scale)', log=True)
        for i, bucket in enumerate(BUCKET_ORDER):
            for j, algo in enumerate(ALGOS):
                data = df[(df['range_bucket']==bucket) &
                          (df['algo_label']==algo)]['avg_query_time_ms'].dropna().values
                data = data[data > 0]
                if len(data) == 0: continue
                pos = group_centers[i] + (j - (n_algos-1)/2) * box_width
                bp = ax.boxplot(data, positions=[pos], widths=box_width*0.8,
                                patch_artist=False, manage_ticks=False)
                style_box(bp, ALGO_COLORS[algo])
        ax.set_xticks(group_centers)
        ax.set_xticklabels([BUCKET_LABELS[b] for b in BUCKET_ORDER],
                           color='black', fontsize=20)

# ── Save ──────────────────────────────────────────────────────────────────────
base     = os.path.splitext(os.path.basename(csv_path))[0]
out_name = f"{base}_{metric_code}.png"
out_path = os.path.join(os.path.dirname(os.path.abspath(csv_path)), out_name)

plt.tight_layout(pad=2.0)
plt.savefig(out_path, dpi=150, bbox_inches='tight', facecolor='white')
print(f"Saved: {out_path}")
