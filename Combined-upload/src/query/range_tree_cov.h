#pragma once
#include "../trees/range_tree.h"
#include "../common/coverage.h"

// Range Tree query with combined coverage + fairness constraints.
optional<Result> queryBestRT_cov(Data &data, RangeTree &tree,
                                  const CoverageData &cov,
                                  double startVal, double endVal,
                                  double alpha, double beta,
                                  const vector<int> &required);
