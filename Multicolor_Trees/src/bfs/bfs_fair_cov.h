#pragma once
#include "../common/common.h"
#include "../common/coverage.h"

// BFS fair range query satisfying both fairness (max-min <= epsilon)
// and coverage (each color >= required minimum) using Tversky similarity.

struct BFSResult {
    double leftVal, rightVal, similarity;
};

optional<BFSResult> bfsFairCov(
    const Data         &data,
    const CoverageData &cov,
    double startVal, double endVal,
    int    epsilon,          // fairness: max_count - min_count <= epsilon
    double alpha, double beta,
    const vector<int>  &required  // coverage: color[c] >= required[c]
);
