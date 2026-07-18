#pragma once
#include "../trees/range_tree.h"
#include "../common/coverage.h"

// Optimized Range Tree query with Tversky-based pruning + coverage constraints.
optional<Result> queryBestRT_opt_cov(Data &data, RangeTree &tree,
                                     const CoverageData &cov,
                                     double startVal, double endVal,
                                     double alpha, double beta,
                                     const vector<int> &required);
