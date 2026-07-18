#pragma once
#include "../trees/rtree.h"
#include "../common/coverage.h"

// R-tree query with combined coverage + fairness constraints.
optional<Result> queryBestRT_rtree_cov(Data &data, RTree &tree,
                                       const CoverageData &cov,
                                       double startVal, double endVal,
                                       double alpha, double beta,
                                       const vector<int> &required);
