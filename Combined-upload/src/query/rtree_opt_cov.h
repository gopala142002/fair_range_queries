#pragma once
#include "../trees/rtree.h"
#include "../common/coverage.h"

// Optimized R-tree query with Tversky-based pruning + coverage constraints.
optional<Result> queryBestRT_rtree_opt_cov(Data &data, RTree &tree,
                                           const CoverageData &cov,
                                           double startVal, double endVal,
                                           double alpha, double beta,
                                           const vector<int> &required);
