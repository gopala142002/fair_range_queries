#pragma once
#include "../common/common.h"
#include "../common/coverage.h"

// BFS fair range query — fairness only (max-min <= epsilon), Tversky similarity.

struct BFSFairResult {
    double leftVal, rightVal, similarity;
};

optional<BFSFairResult> bfsFair(
    const Data         &data,
    const CoverageData &cov,
    double startVal, double endVal,
    int    epsilon,
    double alpha, double beta
);
