import sys
import os
import json
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import numpy as np

# ── CLI ───────────────────────────────────────────────────────────────────────
if len(sys.argv) < 3:
    print("Usage: python plot_time_sim.py <csv_file> <buckets>")
    print("  buckets: any combination of S, M, L")
    sys.exit(1)

csv_path     = sys.argv[1]
bucket_codes = sys.argv[2].upper()

BUCKET_MAP   = {'S': 'small', 'M': 'medium', 'L': 'large'}
BUCKET_LABEL = {'small': 'Small', 'medium': 'Medium', 'large': 'Large'}
buckets      = [BUCKET_MAP[c] for c in bucket_codes]

df = pd.read_csv(csv_path)

mode_map = {'range_opt': 'Range', 'kd_opt': 'KD', 'rtree_opt': 'R', 'rstar_opt': 'R*'}

def get_algo(row):
    if row['algorithm'] == 'brute': return 'Brute'
    if row['algorithm'] == 'bfs':   return 'BFS'
    if row['algorithm'] == 'trees': return mode_map.get(row['mode'], row['mode'])
    return None

df['algo_label'] = df.apply(get_algo, axis=1)

FIG_BG   = 'white'
AX_BG    = 'white'
GRID_COL = '#BBBBBB'

ALGO_COLORS  = {'Brute': '#E07BE0', 'BFS': '#F4A261', 'KD': '#2EC4B6'}
ALGO_MARKERS = {'Brute': 'o', 'BFS': 's', 'KD': 'D'}

def style_ax(ax, title):
    ax.set_facecolor(AX_BG)
    ax.set_title(title, color='black', fontsize=22, pad=10)
    ax.set_xlabel('Time (ms, log scale)', color='black', fontsize=20)
    ax.set_ylabel('Best Similarity Found', color='black', fontsize=20)
    ax.set_xscale('log')
    ax.set_ylim(-0.05, 1.05)
    ax.tick_params(colors='black')
    ax.xaxis.set_tick_params(labelcolor='black', labelsize=18)
    ax.yaxis.set_tick_params(labelcolor='black', labelsize=18)
    for spine in ax.spines.values():
        spine.set_edgecolor(GRID_COL)
    ax.grid(color=GRID_COL, linestyle='--', linewidth=0.5, zorder=0)

def plot_step(ax, times, sims, color, label, marker):
    t_start = max(times[0]*0.01, 1e-4)
    times = [t_start] + list(times)
    sims  = [0.0]     + list(sims)
    ax.step(times, sims, where='post', color=color, linewidth=2.5,
            label=label, zorder=3)
    ax.scatter(times[1:], sims[1:], color=color, marker=marker,
               s=80, zorder=5, alpha=0.8)

def plot_single(ax, t, s, color, label, marker):
    ax.step([1e-4, t, t], [0.0, 0.0, s], where='post', color=color,
            linewidth=2.5, label=label, linestyle='--', zorder=3)
    ax.scatter(t, s, color=color, marker=marker, s=120, zorder=5)

n_plots = len(buckets)
fig, axes = plt.subplots(1, n_plots, figsize=(10*n_plots, 8))
if n_plots == 1: axes = [axes]
fig.patch.set_facecolor(FIG_BG)

for ax, bucket in zip(axes, buckets):
    style_ax(ax, f'Similarity vs Time — {BUCKET_LABEL[bucket]} Bucket')
    sub   = df[df['range_bucket'] == bucket]
    x_max = max((sub[sub['algo_label']==a].iloc[0]['avg_query_time_ms']
                 for a in ['Brute','BFS','KD']
                 if not sub[sub['algo_label']==a].empty), default=1)

    for algo in ['Brute', 'BFS', 'KD']:
        rows = sub[sub['algo_label'] == algo]
        if rows.empty: continue
        row    = rows.iloc[0]
        color  = ALGO_COLORS[algo]
        marker = ALGO_MARKERS[algo]
        log = []
        pl  = row.get('progress_log', '')
        if isinstance(pl, str) and pl.strip() not in ('', '[]', 'nan'):
            try: log = json.loads(pl)
            except: log = []
        if log:
            times = [e['time_ms'] for e in log] + [row['avg_query_time_ms']]
            sims  = [e['similarity'] for e in log] + [row['similarity']]
            plot_step(ax, times, sims, color, algo, marker)
        else:
            plot_single(ax, row['avg_query_time_ms'], row['similarity'],
                        color, algo, marker)
    ax.set_xlim(left=1e-4, right=x_max*3)

# ── Save ──────────────────────────────────────────────────────────────────────
base     = os.path.splitext(os.path.basename(csv_path))[0]
out_name = f"{base}_{bucket_codes}.png"
out_path = os.path.join(os.path.dirname(os.path.abspath(csv_path)), out_name)

plt.tight_layout(pad=2.0)
plt.savefig(out_path, dpi=150, bbox_inches='tight', facecolor='white')
print(f"Saved: {out_path}")
