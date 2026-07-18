#pragma once
#include "../common/common.h"
#include "../common/coverage.h"

// Brute force O(n^2) fair range query with coverage constraints.
// Fairness:  max_color_count - min_color_count <= epsilon
// Coverage:  each color count >= required[c]
optional<Result> queryBruteFairCov(
    const Data         &data,
    const CoverageData &cov,
    double startVal, double endVal,
    double alpha, double beta,
    const vector<int>  &required
);
