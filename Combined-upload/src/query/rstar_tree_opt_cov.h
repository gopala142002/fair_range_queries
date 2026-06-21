#pragma once
#include "../trees/rstar_tree.h"
#include "../common/coverage.h"

// Optimized R* tree query with Tversky-based pruning + coverage constraints.
optional<Result> queryBestRT_rstar_opt_cov(Data &data, RStarTree &tree,
                                           const CoverageData &cov,
                                           double startVal, double endVal,
                                           double alpha, double beta,
                                           const vector<int> &required);
