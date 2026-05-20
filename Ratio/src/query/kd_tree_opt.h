#ifndef KD_TREE_OPT_H
#define KD_TREE_OPT_H

#include "../common/fair_range.h"
#include "../trees/KD_Tree_Synthetic.h"
#include <optional>
#include <tuple>

// A struct to mirror the Result structure expected by the logic
struct OptResult {
    double min_val;
    double max_val;
    double tversky;
};

// Main entry point for the optimized KD-Tree search algorithm
std::optional<OptResult> queryBestKD_opt(
    const std::vector<DatasetPoint>& dataset, 
    const std::vector<Point>& points_data,
    KDTreeNode* tree_root,
    double startVal, double endVal,
    double alpha, double beta);

#endif
