import os

def parse_constraints_file(filename="constraints.txt"):
    """
    Parses a constraints file with the following format:
    
    Weights of color:
    [NaN or space-separated list of float weights]
    
    Coverage:
    [NaN or space-separated list of integer min counts]
    
    Difference:
    [NaN or color x color matrix (one row per color, starting from color 0)]
    
    Ratio:
    [NaN or color x color matrix (one row per color, starting from color 0)]
    """
    if not os.path.exists(filename):
        return None, False, None, None, None

    with open(filename, 'r') as f:
        content = f.read()
        
    lines = [line.strip() for line in content.split('\n')]
    
    sections = {}
    current_section = None
    section_lines = []
    
    for line in lines:
        if not line:
            continue
        if line.endswith(':'):
            if current_section:
                sections[current_section] = section_lines
            current_section = line[:-1].strip().lower()
            section_lines = []
        else:
            if current_section:
                section_lines.append(line)
    if current_section:
        sections[current_section] = section_lines
        
    def is_nan(val_str):
        return val_str.strip().lower() in ('nan', 'none', 'null')
        
    color_weights = None
    if 'weights of color' in sections:
        sec_lines = sections['weights of color']
        if sec_lines and not is_nan(sec_lines[0]):
            parts = sec_lines[0].split()
            color_weights = {i + 1: float(w) for i, w in enumerate(parts)}
            
    coverage = False
    min_coverage = None
    if 'coverage' in sections:
        sec_lines = sections['coverage']
        if sec_lines and not is_nan(sec_lines[0]):
            parts = sec_lines[0].split()
            min_coverage = {i + 1: int(c) for i, c in enumerate(parts)}
            coverage = True
            
    diff_matrix = None
    if 'difference' in sections:
        sec_lines = sections['difference']
        if sec_lines and not is_nan(sec_lines[0]):
            diff_matrix = {}
            for r_idx, line in enumerate(sec_lines):
                parts = line.split()
                c_a = r_idx + 1
                for c_b_idx, val_str in enumerate(parts):
                    c_b = c_b_idx + 1
                    if not is_nan(val_str):
                        diff_matrix[(c_a, c_b)] = float(val_str)
                        
    ratio_matrix = None
    if 'ratio' in sections:
        sec_lines = sections['ratio']
        if sec_lines and not is_nan(sec_lines[0]):
            ratio_matrix = {}
            for r_idx, line in enumerate(sec_lines):
                parts = line.split()
                c_a = r_idx + 1
                for c_b_idx, val_str in enumerate(parts):
                    c_b = c_b_idx + 1
                    if not is_nan(val_str):
                        ratio_matrix[(c_a, c_b)] = float(val_str)
                        
    return color_weights, coverage, min_coverage, diff_matrix, ratio_matrix

def apply_parsed_constraints(filename="constraints.txt"):
    """
    Helper function to load the constraints file if it exists and return the arguments.
    Returns (color_weights, coverage, min_coverage, diff_matrix, ratio_matrix)
    If the file does not exist, returns defaults (None, False, None, None, None).
    """
    if os.path.exists(filename):
        print(f"Reading constraints from {filename}...")
        return parse_constraints_file(filename)
    else:
        print(f"Constraints file {filename} not found, using default fairness rules.")
        return None, False, None, None, None
