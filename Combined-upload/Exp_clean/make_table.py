import sys
import os
import pandas as pd
import numpy as np

# ── CLI ───────────────────────────────────────────────────────────────────────
if len(sys.argv) < 2 or len(sys.argv) > 3:
    print("Usage: python make_table.py <csv_file> [--preview]")
    sys.exit(1)

csv_path = sys.argv[1]
preview  = len(sys.argv) == 3 and sys.argv[2] == '--preview'

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
df['preprocess_ms'] = df['build_time_ms'].fillna(0) + df['file_load_ms'].fillna(0)

ALGOS          = ['BFS', 'Brute', 'KD']
FAIRNESS_ORDER = ['D', 'R', 'C', 'D+R', 'D+C', 'R+C', 'D+R+C']

df = df[df['algo_label'].isin(ALGOS)]

# ── TLE & aggregation ─────────────────────────────────────────────────────────
tle_check = (
    df.groupby(['fairness_combination', 'algo_label'])['tle']
    .apply(lambda x: (x == 'YES').all())
)

agg = (
    df.groupby(['fairness_combination', 'algo_label'])[
        ['preprocess_ms', 'avg_query_time_ms', 'similarity']
    ]
    .mean()
    .round(3)
)

dataset_name = os.path.splitext(os.path.basename(csv_path))[0]

# ── Helpers ───────────────────────────────────────────────────────────────────
def fmt_ms(v):
    if pd.isna(v): return '--'
    return f'{v/1000:.1f}k' if v >= 1000 else f'{v:.1f}'

def fmt_sim(v):
    if pd.isna(v): return '--'
    return f'{v:.3f}'

def is_tle(fc, algo):
    try:    return tle_check.loc[(fc, algo)]
    except: return False

def get_val(fc, algo, col):
    try:    return agg.loc[(fc, algo), col]
    except: return np.nan

def esc(s):
    return s.replace('+', '$+$')

# similarity for a fairness row: use non-TLE algo value, or TLE if all TLE
def get_sim(fc):
    for algo in ALGOS:
        if not is_tle(fc, algo):
            v = get_val(fc, algo, 'similarity')
            if not pd.isna(v):
                return fmt_sim(v)
    return r'\textit{TLE}' if not preview else 'TLE'

# ── Preview ───────────────────────────────────────────────────────────────────
if preview:
    CW  = 10
    FCW = 10
    SW  = 10  # similarity single column

    sep = '-' * (FCW + CW * 6 + SW)
    print(f"\nDataset: {dataset_name}\n")
    print(sep)
    print(f"{'':>{FCW}}"
          f"{'-- Preprocessing (ms) --':^{CW*3}}"
          f"{'-- Query Time (ms) --':^{CW*3}}"
          f"{'Sim':^{SW}}")
    print(f"{'Fairness':<{FCW}}"
          f"{'BFS':^{CW}}{'Brute':^{CW}}{'KD':^{CW}}"
          f"{'BFS':^{CW}}{'Brute':^{CW}}{'KD':^{CW}}"
          f"{'':^{SW}}")
    print(sep)

    for fc in FAIRNESS_ORDER:
        row = f"{fc:<{FCW}}"
        for algo in ALGOS:
            row += f"{'TLE' if is_tle(fc,algo) else fmt_ms(get_val(fc,algo,'preprocess_ms')):^{CW}}"
        for algo in ALGOS:
            row += f"{'TLE' if is_tle(fc,algo) else fmt_ms(get_val(fc,algo,'avg_query_time_ms')):^{CW}}"
        # single similarity column
        sim_val = 'TLE'
        for algo in ALGOS:
            if not is_tle(fc, algo):
                v = get_val(fc, algo, 'similarity')
                if not pd.isna(v):
                    sim_val = fmt_sim(v)
                    break
        row += f"{sim_val:^{SW}}"
        print(row)

    print(sep)

# ── LaTeX ─────────────────────────────────────────────────────────────────────
else:
    lines = []
    lines.append(r'\begin{table*}[ht]')
    lines.append(r'\centering')
    lines.append(r'\caption{Preprocessing time, query time, and similarity per fairness combination -- '
                 + dataset_name.replace('_', r'\_') + r'}')
    lines.append(r'\label{tab:' + dataset_name + r'}')
    lines.append(r'\setlength{\tabcolsep}{4pt}')
    lines.append(r'\begin{tabular}{l ccc ccc c}')
    lines.append(r'\toprule')
    lines.append(
        r'\textbf{Fairness} & '
        r'\multicolumn{3}{c}{\textbf{Preprocessing (ms)}} & '
        r'\multicolumn{3}{c}{\textbf{Query Time (ms)}} & '
        r'\textbf{Sim.} \\'
    )
    lines.append(r'\cmidrule(lr){2-4}\cmidrule(lr){5-7}')
    lines.append(
        r'\textbf{Criteria} & '
        r'\textbf{BFS} & \textbf{Brute} & \textbf{KD} & '
        r'\textbf{BFS} & \textbf{Brute} & \textbf{KD} & \\'
    )
    lines.append(r'\midrule')

    for fc in FAIRNESS_ORDER:
        cells = [esc(fc)]
        for algo in ALGOS:
            cells.append(r'\textit{TLE}' if is_tle(fc, algo)
                         else fmt_ms(get_val(fc, algo, 'preprocess_ms')))
        for algo in ALGOS:
            cells.append(r'\textit{TLE}' if is_tle(fc, algo)
                         else fmt_ms(get_val(fc, algo, 'avg_query_time_ms')))
        cells.append(get_sim(fc))
        lines.append(' & '.join(cells) + r' \\')

    lines.append(r'\bottomrule')
    lines.append(r'\end{tabular}')
    lines.append(r'\end{table*}')

    latex = '\n'.join(lines)
    out_path = os.path.join(os.path.dirname(os.path.abspath(csv_path)),
                            dataset_name + '_table.tex')
    with open(out_path, 'w') as f:
        f.write(latex)
    print(f"Saved: {out_path}")
