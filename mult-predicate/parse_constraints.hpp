#ifndef PARSE_CONSTRAINTS_HPP
#define PARSE_CONSTRAINTS_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cmath>

struct PairwiseConstraint {
    int type = 0; // 0: None, 1: Ratio, 2: Difference
    double threshold = 0.0;
    double weight_i = 1.0;
    double weight_j = 1.0;
};

inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if(start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

inline bool is_nan_str(const std::string& s) {
    std::string lower = s;
    for(auto& c : lower) c = tolower(c);
    return lower == "nan" || lower == "none" || lower == "null";
}

inline void parse_constraints_file(const std::string& filename,
                                   int numColors,
                                   std::vector<double>& color_weights,
                                   bool& use_coverage,
                                   std::vector<int>& min_coverage,
                                   std::vector<std::vector<PairwiseConstraint>>& constraints_matrix) {
                                       
    constraints_matrix.assign(numColors, std::vector<PairwiseConstraint>(numColors));
    color_weights.assign(numColors, 1.0);
    min_coverage.assign(numColors, 0);
    use_coverage = false;

    std::ifstream cfin(filename);
    if (!cfin.is_open()) {
        std::cout << "Constraints file " << filename << " not found, using default fairness rules.\n";
        // Default to difference constraint with threshold 0 for everything
        for (int i = 0; i < numColors; ++i) {
            for (int j = i + 1; j < numColors; ++j) {
                constraints_matrix[i][j].type = 2; // Difference
                constraints_matrix[i][j].threshold = 0.0; // default epsilon
                constraints_matrix[j][i].type = 2;
                constraints_matrix[j][i].threshold = 0.0;
            }
        }
        return;
    }

    std::map<std::string, std::vector<std::string>> sections;
    std::string current_section = "";
    std::string line;

    while (std::getline(cfin, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.back() == ':') {
            current_section = line.substr(0, line.size() - 1);
            for(auto& c : current_section) c = tolower(c);
            continue;
        }

        if (!current_section.empty()) {
            sections[current_section].push_back(line);
        }
    }

    // Parse Weights
    if (sections.count("weights of color")) {
        auto& sec = sections["weights of color"];
        if (!sec.empty() && !is_nan_str(sec[0])) {
            std::stringstream ss(sec[0]);
            std::string val;
            int idx = 1;
            while (ss >> val && idx < numColors) {
                if (!is_nan_str(val)) {
                    color_weights[idx] = std::stod(val);
                }
                idx++;
            }
        }
    }

    // Parse Coverage
    if (sections.count("coverage")) {
        auto& sec = sections["coverage"];
        if (!sec.empty() && !is_nan_str(sec[0])) {
            use_coverage = true;
            std::stringstream ss(sec[0]);
            std::string val;
            int idx = 1;
            while (ss >> val && idx < numColors) {
                if (!is_nan_str(val)) {
                    min_coverage[idx] = std::stoi(val);
                }
                idx++;
            }
        }
    }

    // Parse Difference
    if (sections.count("difference")) {
        auto& sec = sections["difference"];
        if (!sec.empty() && !is_nan_str(sec[0])) {
            for (size_t r_idx = 0; r_idx < sec.size(); ++r_idx) {
                int c_a = r_idx + 1;
                if (c_a >= numColors) break;
                
                std::stringstream ss(sec[r_idx]);
                std::string val;
                int c_b_idx = 0;
                while (ss >> val) {
                    int c_b = c_b_idx + 1;
                    if (c_b >= numColors) break;
                    if (!is_nan_str(val)) {
                        constraints_matrix[c_a][c_b].type = 2; // Difference
                        constraints_matrix[c_a][c_b].threshold = std::stod(val);
                        constraints_matrix[c_a][c_b].weight_i = color_weights[c_a];
                        constraints_matrix[c_a][c_b].weight_j = color_weights[c_b];
                    }
                    c_b_idx++;
                }
            }
        }
    }

    // Parse Ratio
    if (sections.count("ratio")) {
        auto& sec = sections["ratio"];
        if (!sec.empty() && !is_nan_str(sec[0])) {
            for (size_t r_idx = 0; r_idx < sec.size(); ++r_idx) {
                int c_a = r_idx + 1;
                if (c_a >= numColors) break;
                
                std::stringstream ss(sec[r_idx]);
                std::string val;
                int c_b_idx = 0;
                while (ss >> val) {
                    int c_b = c_b_idx + 1;
                    if (c_b >= numColors) break;
                    if (!is_nan_str(val)) {
                        // Ratio overrides difference if specified
                        constraints_matrix[c_a][c_b].type = 1; // Ratio
                        constraints_matrix[c_a][c_b].threshold = std::stod(val);
                        constraints_matrix[c_a][c_b].weight_i = color_weights[c_a];
                        constraints_matrix[c_a][c_b].weight_j = color_weights[c_b];
                    }
                    c_b_idx++;
                }
            }
        }
    }
}

#endif
