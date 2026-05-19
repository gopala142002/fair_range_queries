#include "brute_fair_cov.h"

optional<Result> queryBruteFairCov(
    const Data         &data,
    const CoverageData &cov,
    int    epsilon,
    double startVal, double endVal,
    double alpha, double beta,
    const vector<int>  &required)
{
    int n = data.n;
    if (n == 0) return nullopt;

    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0 = max(L0, 0); R0 = min(R0, n-1);
    if (L0 > R0) return nullopt;

    int qL = L0 + 1, qR = R0 + 1;
    int numColors = (int)cov.colorPrefix.size();

    double bestT = -1.0;
    int bestL = -1, bestR = -1;

    // O(n^2): check every possible range [L, R]
    for (int L = 1; L <= n; ++L) {
        for (int R = L; R <= n; ++R) {

            // Get count of each color in [L, R]
            // count[c] = colorPrefix[c][R] - colorPrefix[c][L-1]
            int minCnt = INT_MAX, maxCnt = INT_MIN;
            bool coverageOk = true;

            for (int c = 0; c < numColors; ++c) {
                int cnt = cov.colorPrefix[c][R] - cov.colorPrefix[c][L-1];

                // Fairness tracking
                minCnt = min(minCnt, cnt);
                maxCnt = max(maxCnt, cnt);

                // Coverage check
                if (cnt < required[c]) { coverageOk = false; break; }
            }

            // Both must be satisfied
            if (!coverageOk) continue;
            if (maxCnt - minCnt > epsilon) continue;

            // Compute Tversky similarity
            double t = tversky(L, R, qL, qR, alpha, beta);

            if (t > bestT) {
                bestT = t; bestL = L; bestR = R;
            } else if (t == bestT && t >= 0.0) {
                int wBest = bestR - bestL, wCur = R - L;
                if (wCur < wBest ||
                   (wCur == wBest && L < bestL) ||
                   (wCur == wBest && L == bestL && R < bestR)) {
                    bestL = L; bestR = R;
                }
            }
        }
    }

    if (bestL == -1) return nullopt;
    return Result{data.keys[bestL-1], data.keys[bestR-1], bestT};
}
