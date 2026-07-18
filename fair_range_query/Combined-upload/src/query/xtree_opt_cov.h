#pragma once
#include "../trees/xtree.h"
#include "../common/coverage.h"

// Optimized X-tree query with Tversky-based pruning + coverage constraints.
optional<Result> queryBestXT_opt_cov(Data &data, XTree &tree,
                                     const CoverageData &cov,
                                     double startVal, double endVal,
                                     double alpha, double beta,
                                     const vector<int> &required);
