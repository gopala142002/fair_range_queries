#pragma once
#include "../trees/xtree.h"
#include "../common/coverage.h"

// X-tree query with combined coverage + fairness constraints.
optional<Result> queryBestXT_cov(Data &data, XTree &tree,
                                  const CoverageData &cov,
                                  double startVal, double endVal,
                                  double alpha, double beta,
                                  const vector<int> &required);
