#pragma once
#include "../trees/kd_tree.h"

// Optimized KD-tree query with Tversky-based pruning and smart side selection.
optional<Result> queryBestKD_opt(const Data &data, const KDTree &tree,
                                 double startVal, double endVal,
                                 double alpha, double beta);
