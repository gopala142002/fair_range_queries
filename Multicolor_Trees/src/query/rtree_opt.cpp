#include "rtree_opt.h"

static void updateResult(int L, int R, int qL, int qR,
                         double alpha, double beta,
                         double &bestT, int &bestL, int &bestR) {
    double t = tversky(L, R, qL, qR, alpha, beta);
    if (t > bestT) {
        bestT = t; bestL = L; bestR = R;
    } else if (t == bestT && t >= 0.0) {
        int wBest = bestR - bestL, wCur = R - L;
        if (wCur < wBest || (wCur == wBest && L < bestL) ||
            (wCur == wBest && L == bestL && R < bestR)) {
            bestL = L; bestR = R;
        }
    }
}

// Tversky upper bounds — same as range_tree_opt
static double ubLeftPhase1(int L, int qL, int qR, double beta) {
    double Q = qR - qL + 1;
    return Q / (Q + beta * (qL - L));
}
static double ubLeftPhase2(int L, int qL, int qR, double alpha) {
    double inter = qR - L + 1, onlyIn = L - qL;
    return inter / (inter + alpha * onlyIn);
}
static double ubRightPhase1(int R, int qL, int qR, double beta) {
    double Q = qR - qL + 1;
    return Q / (Q + beta * (R - qR));
}
static double ubRightPhase2(int R, int qL, int qR, double alpha) {
    double inter = R - qL + 1, onlyIn = qR - R;
    return inter / (inter + alpha * onlyIn);
}

optional<Result> queryBestRT_rtree_opt(Data &data, RTree &tree,
                                       int epsilon, double startVal, double endVal,
                                       double alpha, double beta) {
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

    auto tryLeft = [&](int p) {
        const auto &center = data.points[p];
        for (int d = 0; d < data.m; ++d) { lo[d] = center[d]-epsilon; hi[d] = center[d]+epsilon; }
        lo[data.m] = p + 1; hi[data.m] = n;
        int bestRforP = tree.findClosestIndexToTarget(lo, hi, qR);
        if (bestRforP == -1) return;
        updateResult(p + 1, bestRforP, qL, qR, alpha, beta, bestT, bestL, bestR);
    };

    auto tryRight = [&](int r) {
        const auto &center = data.points[r];
        for (int d = 0; d < data.m; ++d) { lo[d] = center[d]-epsilon; hi[d] = center[d]+epsilon; }
        lo[data.m] = 1; hi[data.m] = r - 1;
        int bestLforR = tree.findClosestIndexToTarget(lo, hi, qL);
        if (bestLforR == -1) return;
        updateResult(bestLforR, r, qL, qR, alpha, beta, bestT, bestL, bestR);
    };

    if (qR <= n - qL) {
        // Left side smaller
        // Phase 1: L goes left from qL toward 0
        for (int p = qL - 2; p >= 0; --p) {
            if (ubLeftPhase1(p + 1, qL, qR, beta) <= bestT) break;
            tryLeft(p);
        }
        // Phase 2: L goes right from qL toward qR
        for (int p = qL - 1; p <= qR - 1; ++p) {
            if (ubLeftPhase2(p + 1, qL, qR, alpha) <= bestT) break;
            tryLeft(p);
        }
    } else {
        // Right side smaller
        // Phase 1: R goes right from qR toward n
        for (int r = qR; r <= n; ++r) {
            if (ubRightPhase1(r, qL, qR, beta) <= bestT) break;
            tryRight(r);
        }
        // Phase 2: R goes left from qR toward qL
        for (int r = qR - 1; r >= qL; --r) {
            if (ubRightPhase2(r, qL, qR, alpha) <= bestT) break;
            tryRight(r);
        }
    }

    if (bestL == -1) return nullopt;
    return Result{data.keys[bestL - 1], data.keys[bestR - 1], bestT};
}
