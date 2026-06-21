import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.lines as mlines
import os

OUT_DIR = os.path.dirname(os.path.abspath(__file__))

ALGO_COLORS  = {'Brute': '#E07BE0', 'BFS': '#F4A261', 'Range': '#4C9BE8',
                'KD': '#2EC4B6', 'R': '#E63946', 'R*': '#A8DADC'}
ALGO_MARKERS = {'Brute': 'o', 'BFS': 's', 'Range': '^',
                'KD': 'D', 'R': 'v', 'R*': 'P'}
BUCKET_COLORS = {'Small': '#4C9BE8', 'Medium': '#F4A261', 'Large': '#2EC4B6'}
AB_COLORS = {'α=0.2 β=0.8': '#4C9BE8', 'α=0.4 β=0.6': '#F4A261',
             'α=0.6 β=0.4': '#2EC4B6', 'α=0.8 β=0.2': '#E07BE0'}

def save_legend(handles, filename, ncol):
    fig, ax = plt.subplots(figsize=(max(4, ncol * 2.2), 0.7))
    ax.axis('off')
    legend = ax.legend(handles=handles, loc='center', ncol=ncol,
                       fontsize=18, frameon=True, edgecolor='#BBBBBB',
                       handlelength=2, handleheight=1.5)
    fig.patch.set_facecolor('white')
    path = os.path.join(OUT_DIR, filename)
    plt.savefig(path, dpi=150, bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"Saved: {path}")

# 1. legend_algos — for Q box plots and sensitivity (6 algorithms)
handles = [mpatches.Patch(color=ALGO_COLORS[a], label=a)
           for a in ['Brute', 'BFS', 'Range', 'KD', 'R', 'R*']]
save_legend(handles, 'legend_algos.png', ncol=6)

# 2. legend_buckets — for S box plots (3 buckets)
handles = [mpatches.Patch(color=BUCKET_COLORS[b], label=b)
           for b in ['Small', 'Medium', 'Large']]
save_legend(handles, 'legend_buckets.png', ncol=3)

# 3. legend_ab — for alpha-beta plots (4 combinations)
handles = [mpatches.Patch(color=AB_COLORS[ab], label=ab)
           for ab in AB_COLORS]
save_legend(handles, 'legend_ab.png', ncol=4)

# 4. legend_convergence — for time-sim plots (3 algorithms)
handles = [
    mlines.Line2D([], [], color=ALGO_COLORS['KD'], marker=ALGO_MARKERS['KD'],
                  linewidth=2, markersize=8, label='KD (anytime)'),
    mlines.Line2D([], [], color=ALGO_COLORS['Brute'], marker=ALGO_MARKERS['Brute'],
                  linewidth=2, linestyle='--', markersize=8, label='Brute'),
    mlines.Line2D([], [], color=ALGO_COLORS['BFS'], marker=ALGO_MARKERS['BFS'],
                  linewidth=2, linestyle='--', markersize=8, label='BFS'),
]
save_legend(handles, 'legend_convergence.png', ncol=3)

print("\nAll legends generated in legend/ folder.")
