#include "brute_fair.h"

optional<Result> queryBruteFair(
    const Data         &data,
    double startVal, double endVal,
    double alpha, double beta)
{
    auto search_start_time = std::chrono::high_resolution_clock::now();
    int n = data.n;
    if (n == 0) return nullopt;

    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0 = max(L0, 0); R0 = min(R0, n-1);
    if (L0 > R0) return nullopt;

    int qL = L0 + 1, qR = R0 + 1;

    double bestT = -1.0;
    int bestL = -1, bestR = -1;

    // O(n^2): check every possible range [L, R]
    for (int L = 1; L <= n; ++L) {
        for (int R = L; R <= n; ++R) {

            if (!isFair(data, L, R)) continue;

            // Compute Tversky similarity
            double t = tversky(L, R, qL, qR, alpha, beta);

            if (t > bestT) {
        auto current_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(current_time - search_start_time).count();
        std::cout << "[Progress] Time: " << std::fixed << std::setprecision(4) << elapsed_ms << " ms, Similarity: " << t << "\n";
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
