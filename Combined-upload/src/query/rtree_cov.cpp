#include "rtree_cov.h"

optional<Result> queryBestRT_rtree_cov(Data &data, RTree &tree,
                                       const CoverageData &cov,
                                       double startVal, double endVal,
                                       double alpha, double beta,
                                       const vector<int> &required) {
    auto search_start_time = std::chrono::high_resolution_clock::now();
    int n = data.n;
    if (n == 0) return nullopt;

    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0 = max(L0, 0); R0 = min(R0, n - 1);
    if (L0 > R0) return nullopt;

    int qL = L0 + 1, qR = R0 + 1;
    double bestT = -1.0;
    int bestL = -1, bestR = -1;

    vector<double> lo(data.m + 1), hi(data.m + 1);

    for (int p = 0; p < n; ++p) {
        // Step 1: find coverage point — minimum R satisfying all color counts
        int coveragePoint = findCoveragePoint(cov, p, required, n);
        if (coveragePoint == -1) continue;

        // Step 2: find R closest to qR at or after coveragePoint satisfying fairness
        const auto &center = data.points[p];
        setQueryBounds(data, center, lo, hi);
        lo[data.m] = coveragePoint;  // R must be >= coveragePoint
        hi[data.m] = n;

        int bestRforP = tree.findClosestIndexToTarget(lo, hi, qR);
        if (bestRforP == -1) continue;

        int L = p + 1, R = bestRforP;
        double t = tversky(L, R, qL, qR, alpha, beta);

        if (t > bestT) {
        auto current_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(current_time - search_start_time).count();
        std::cout << "[Progress] Time: " << std::fixed << std::setprecision(4) << elapsed_ms << " ms, Similarity: " << t << "\n";
            bestT = t; bestL = L; bestR = R;
        } else if (t == bestT && t >= 0.0) {
            int wBest = bestR - bestL, wCur = R - L;
            if (wCur < wBest || (wCur == wBest && L < bestL) ||
                (wCur == wBest && L == bestL && R < bestR)) {
                bestL = L; bestR = R;
            }
        }
    }

    if (bestL == -1) return nullopt;
    return Result{data.keys[bestL - 1], data.keys[bestR - 1], bestT};
}
