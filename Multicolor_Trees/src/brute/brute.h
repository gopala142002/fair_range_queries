#pragma once
#include "../common/common.h"

// Brute force O(n^2) fair range query using Tversky similarity.
optional<Result> queryBrute(const Data &data, int epsilon,
                            double startVal, double endVal,
                            double alpha, double beta);
