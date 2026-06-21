#pragma once
#include "../trees/kd_tree.h"
#include "../common/coverage.h"

// Optimized KD-Tree query with Tversky-based pruning + coverage constraints.
optional<Result> queryBestKD_opt_cov(const Data &data, const KDTree &tree,
                                     const CoverageData &cov,
                                     double startVal, double endVal,
                                     double alpha, double beta,
                                     const vector<int> &required);
