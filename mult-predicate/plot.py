import matplotlib.pyplot as plt
import matplotlib.lines as mlines

# 1. Define the parsed data from the experiment logs
data = {
    'Bucket b1 (20%-40%)': {
        'Dinkelbach WS': ([0, 13.82, 805.63, 891.22, 919.53, 953.68, 980.86, 980.96], 
                          [0, 0.2235, 0.9236, 0.9590, 0.9590, 0.9663, 0.9663, 0.9663]),
        'Dinkelbach NoWS': ([0, 115.67, 1800], 
                            [0, 0.2345, 0.2345]),
        'Binary WS': ([0, 175.91, 365.24, 473.59, 509.75, 585.48, 871.59, 1055.09, 1483.71], 
                      [0, 0.5341, 0.7903, 0.8954, 0.9480, 0.9590, 0.9641, 0.9663, 0.9663]),
        'Binary NoWS': ([0, 185.87, 1113.62, 1303.21, 1674.97, 1800], 
                        [0, 0.5014, 0.7590, 0.8800, 0.9437, 0.9437]),
        'BFSMP': ([], []) 
    },
    'Bucket b2 (40%-60%)': {
        'Dinkelbach WS': ([0, 14.08, 214.53, 250.46, 1305.09, 1305.44], 
                          [0, 0.5427, 0.9711, 0.9711, 0.9720, 0.9720]),
        'Dinkelbach NoWS': ([0, 60.49, 1800], 
                            [0, 0.5427, 0.5427]),
        'Binary WS': ([0, 136.39, 365.57, 504.18, 573.27, 666.79, 716.64, 753.09, 825.99, 1800], 
                      [0, 0.5427, 0.7736, 0.8869, 0.9445, 0.9585, 0.9656, 0.9706, 0.9711, 0.9711]),
        'Binary NoWS': ([0, 73.63, 403.86, 1800], 
                        [0, 0.5427, 0.7714, 0.7714]),
        'BFSMP': ([], [])
    },
    'Bucket b3 (60%-80%)': {
        'Dinkelbach WS': ([0, 13.44, 148.47, 197.95, 1800], 
                          [0, 0.7093, 0.9454, 0.9454, 0.9454]),
        'Dinkelbach NoWS': ([0, 72.79, 1800], 
                            [0, 0.7093, 0.7093]),
        'Binary WS': ([0, 112.71, 229.75, 295.07, 426.32, 530.68, 1800], 
                      [0, 0.7093, 0.8571, 0.9318, 0.9442, 0.9454, 0.9454]),
        'Binary NoWS': ([0, 85.13, 429.03, 1800], 
                        [0, 0.7093, 0.8800, 0.8800]),
        'BFSMP': ([], [])
    },
    'Bucket b4 (80%-100%)': {
        'Dinkelbach WS': ([0, 302.13, 325.43, 340.15, 349.16, 349.19], 
                          [0, 0.9788, 0.9890, 0.9890, 0.9918, 0.9918]),
        'Dinkelbach NoWS': ([0, 235.06, 725.05, 1153.72, 1153.97], 
                            [0, 0.9071, 0.9918, 0.9918, 0.9918]),
        'Binary WS': ([0, 170.07, 216.26, 239.48, 268.14, 297.53, 312.45, 356.96, 374.45, 548.53], 
                      [0, 0.9030, 0.9556, 0.9828, 0.9873, 0.9884, 0.9889, 0.9890, 0.9918, 0.9918]),
        'Binary NoWS': ([0, 86.01, 1800], 
                        [0, 0.9030, 0.9030]),
        'BFSMP': ([], [])
    }
}

# 2. Define Plot Configurations
colors = {
    'Dinkelbach WS': 'deepskyblue',  
    'Dinkelbach NoWS': 'blue',       
    'Binary WS': 'red',              
    'Binary NoWS': 'green',          
    'BFSMP': '#DAA520'               
}

fig, axes = plt.subplots(1, 4, figsize=(24, 6))

# 3. Plotting Loop
for ax, (bucket_name, algos) in zip(axes, data.items()):
    ax.set_title(bucket_name)
    ax.set_xlabel('Time (s)')
    
    # Only show the y-label on the first (left-most) plot to reduce clutter
    if ax == axes[0]:
        ax.set_ylabel('Best Similarity Found')
        
    ax.set_ylim(-0.05, 1.05)
    ax.set_xlim(0, 1850) 
    
    tle_text_offset = 0 
    
    for algo_name, (times, sims) in algos.items():
        color = colors[algo_name]
        
        if not times:
            ax.text(0.05, 0.95 - tle_text_offset, f"{algo_name}: TLE", 
                    color=color, transform=ax.transAxes, 
                    fontsize=10, fontweight='bold',
                    bbox=dict(facecolor='white', alpha=0.8, edgecolor=color, boxstyle='round,pad=0.2'))
            tle_text_offset += 0.08 
        else:
            ax.step(times, sims, where='post', color=color, linewidth=2, label=algo_name)
            ax.plot(times[1:], sims[1:], 'o', color=color, markersize=4)

    ax.grid(True, linestyle='--', alpha=0.5)

# 4. Custom Legend Fix
legend_elements = []
for algo_name, color in colors.items():
    line = mlines.Line2D([], [], color=color, marker='None', markersize=5, 
                         linewidth=3, label=algo_name)
    legend_elements.append(line)

# Position the legend safely inside the bottom space
leg = fig.legend(handles=legend_elements, loc='center', ncol=5, 
                 bbox_to_anchor=(0.5, 0.08), frameon=False, fontsize=13)

for text, algo_name in zip(leg.get_texts(), colors.keys()):
    text.set_color(colors[algo_name])
    text.set_weight('bold')

# Set layout constraints. 
# top=0.95 pulls the plots up now that the title is gone.
plt.tight_layout()
fig.subplots_adjust(left=0.04, right=0.98, bottom=0.20, top=0.95, wspace=0.25) 

plt.show()