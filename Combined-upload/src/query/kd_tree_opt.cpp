#include "kd_tree_opt.h"

static void updateResult(int L, int R, int qL, int qR,
                         double alpha, double beta,
                         double &bestT, int &bestL, int &bestR, const std::chrono::high_resolution_clock::time_point& search_start_time) {
    double t = tversky(L, R, qL, qR, alpha, beta);
    if (t > bestT) {
        auto current_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(current_time - search_start_time).count();
        std::cout << "[Progress] Time: " << std::fixed << std::setprecision(4) << elapsed_ms << " ms, Similarity: " << t << "\n";
        bestT=t; bestL=L; bestR=R;
    } else if (t==bestT && t>=0.0) {
        int wBest=bestR-bestL, wCur=R-L;
        if (wCur<wBest||(wCur==wBest&&L<bestL)||(wCur==wBest&&L==bestL&&R<bestR)) {
            bestL=L; bestR=R;
        }
    }
}

static double upperBoundLeftPhase1(int L, int qL, int qR, double alpha) {
    double Q = qR - qL + 1;
    return Q / (Q + alpha * (qL - L));
}

static double upperBoundLeftPhase2(int L, int qL, int qR, double beta) {
    double inter  = qR - L + 1;
    double onlyIn = L - qL;
    return inter / (inter + beta * onlyIn);
}

static double upperBoundRightPhase1(int R, int qL, int qR, double alpha) {
    double Q = qR - qL + 1;
    return Q / (Q + alpha * (R - qR));
}

static double upperBoundRightPhase2(int R, int qL, int qR, double beta) {
    double inter  = R - qL + 1;
    double onlyIn = qR - R;
    return inter / (inter + beta * onlyIn);
}

optional<Result> queryBestKD_opt(const Data &data, const KDTree &tree,
                                 double startVal, double endVal,
                                 double alpha, double beta) {
    auto search_start_time = std::chrono::high_resolution_clock::now();
    int n = data.n;
    if (n == 0) return nullopt;

    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0=max(L0,0); R0=min(R0,n-1);
    if (L0>R0) return nullopt;

    int qL=L0+1, qR=R0+1;
    double bestT=-1.0; int bestL=-1, bestR=-1;
    vector<double> lo(data.m+1), hi(data.m+1);

    auto tryLeft = [&](int p) {
        const auto &center = data.points[p];
        setQueryBounds(data, center, lo, hi);
        lo[data.m] = p + 1; hi[data.m] = n;
        auto [bestRforP_1, bestRforP_2] = getBothClosest(tree, lo, hi, qR, data.m);
        if (bestRforP_1 != -1) { int bestRforP = bestRforP_1; updateResult(p + 1, bestRforP, qL, qR, alpha, beta, bestT, bestL, bestR, search_start_time); }
        if (bestRforP_2 != -1) { int bestRforP = bestRforP_2; updateResult(p + 1, bestRforP, qL, qR, alpha, beta, bestT, bestL, bestR, search_start_time); }
    };

    auto tryRight = [&](int r) {
        const auto &center = data.points[r];
        setQueryBounds(data, center, lo, hi, true);
        lo[data.m] = 0; hi[data.m] = r - 1;
        auto [bestLforR_1, bestLforR_2] = getBothClosest(tree, lo, hi, qL - 1, data.m);
        if (bestLforR_1 != -1) { int bestLforR = bestLforR_1 + 1; updateResult(bestLforR, r, qL, qR, alpha, beta, bestT, bestL, bestR, search_start_time); }
        if (bestLforR_2 != -1) { int bestLforR = bestLforR_2 + 1; updateResult(bestLforR, r, qL, qR, alpha, beta, bestT, bestL, bestR, search_start_time); }
    };

    if (qR <= n - qL) {

        // ---- LEFT SIDE smaller ----

        // Phase 1: L goes LEFT from qL toward 0 with pruning
        for (int p = qL-2; p >= 0; --p) {
            int L = p+1;
            if (upperBoundLeftPhase1(L, qL, qR, alpha) <= bestT) break;
            tryLeft(p);
        }

        // Phase 2: L goes RIGHT from qL toward qR with pruning
        for (int p = qL-1; p <= qR-1; ++p) {
            int L = p+1;
            if (upperBoundLeftPhase2(L, qL, qR, beta) <= bestT) break;
            tryLeft(p);
        }

    } else {

        // ---- RIGHT SIDE smaller ----

        // Phase 1: R goes RIGHT from qR toward n with pruning
        for (int r = qR; r <= n; ++r) {
            if (upperBoundRightPhase1(r, qL, qR, alpha) <= bestT) break;
            tryRight(r);
        }

        // Phase 2: R goes LEFT from qR toward qL with pruning
        for (int r = qR-1; r >= qL; --r) {
            if (upperBoundRightPhase2(r, qL, qR, beta) <= bestT) break;
            tryRight(r);
        }
    }

    if (bestL==-1) return nullopt;
    return Result{data.keys[bestL-1], data.keys[bestR-1], bestT};
}
