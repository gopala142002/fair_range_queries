#pragma once
#include "../common/common.h"
#include "../common/coverage.h"

// Brute force O(n^2) fair range query.
// Fairness: max_color_count - min_color_count <= epsilon
// Uses individual color prefix sums from CoverageData.
optional<Result> queryBruteFair(
    const Data         &data,
    double startVal, double endVal,
    double alpha, double beta
);
