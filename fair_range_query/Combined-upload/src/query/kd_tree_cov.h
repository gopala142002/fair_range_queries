#pragma once
#include "../trees/kd_tree.h"
#include "../common/coverage.h"

// KD-Tree query with combined coverage + fairness constraints.
optional<Result> queryBestKD_cov(const Data &data, const KDTree &tree,
                                  const CoverageData &cov,
                                  double startVal, double endVal,
                                  double alpha, double beta,
                                  const vector<int> &required);
