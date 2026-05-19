#pragma once
#include "../trees/rstar_tree.h"
#include "../common/coverage.h"

// R* tree query with combined coverage + fairness constraints.
optional<Result> queryBestRT_rstar_cov(Data &data, RStarTree &tree,
                                       const CoverageData &cov,
                                       int epsilon, double startVal, double endVal,
                                       double alpha, double beta,
                                       const vector<int> &required);
