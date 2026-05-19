#include "kd_tree_opt.h"

static void updateResult(int L, int R, int qL, int qR,
                         double alpha, double beta,
                         double &bestT, int &bestL, int &bestR) {
    double t = tversky(L, R, qL, qR, alpha, beta);
    if (t > bestT) {
        bestT=t; bestL=L; bestR=R;
    } else if (t==bestT && t>=0.0) {
        int wBest=bestR-bestL, wCur=R-L;
        if (wCur<wBest||(wCur==wBest&&L<bestL)||(wCur==wBest&&L==bestL&&R<bestR)) {
            bestL=L; bestR=R;
        }
    }
}

static double upperBoundLeftPhase1(int L, int qL, int qR, double beta) {
    double Q = qR - qL + 1;
    return Q / (Q + beta * (qL - L));
}

static double upperBoundLeftPhase2(int L, int qL, int qR, double alpha) {
    double inter  = qR - L + 1;
    double onlyIn = L - qL;
    return inter / (inter + alpha * onlyIn);
}

static double upperBoundRightPhase1(int R, int qL, int qR, double beta) {
    double Q = qR - qL + 1;
    return Q / (Q + beta * (R - qR));
}

static double upperBoundRightPhase2(int R, int qL, int qR, double alpha) {
    double inter  = R - qL + 1;
    double onlyIn = qR - R;
    return inter / (inter + alpha * onlyIn);
}

optional<Result> queryBestKD_opt(const Data &data, const KDTree &tree,
                                 int epsilon, double startVal, double endVal,
                                 double alpha, double beta) {
    int n = data.n;
    if (n == 0) return nullopt;

    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0=max(L0,0); R0=min(R0,n-1);
    if (L0>R0) return nullopt;

    int qL=L0+1, qR=R0+1;

    // Fixed 6 candidate R values near qL and qR (KD-tree approach)
    set<int> candSet;
    for (int v : {qL-1, qL, qL+1, qR-1, qR, qR+1})
        if (v>=1 && v<=n) candSet.insert(v);
    vector<int> candRs(candSet.begin(), candSet.end());

    double bestT=-1.0; int bestL=-1, bestR=-1;
    vector<double> lo(data.m+1), hi(data.m+1);

    // Try fixed left boundary p — check all candidate R values
    auto tryLeft = [&](int p) {
        const auto &center = data.points[p];
        for (int d=0; d<data.m; ++d) { lo[d]=center[d]-epsilon; hi[d]=center[d]+epsilon; }
        for (int candR : candRs) {
            if (candR <= p) continue;
            lo[data.m] = hi[data.m] = (double)candR;
            if (!tree.pointExistsInRange(lo, hi)) continue;
            updateResult(p+1, candR, qL, qR, alpha, beta, bestT, bestL, bestR);
        }
    };

    // Try fixed right boundary r — check all candidate L values near qL/qR
    auto tryRight = [&](int r) {
        const auto &center = data.points[r];
        for (int d=0; d<data.m; ++d) { lo[d]=center[d]-epsilon; hi[d]=center[d]+epsilon; }
        set<int> candLSet;
        for (int v : {qL-1, qL, qL+1, qR-1, qR, qR+1})
            if (v>=1 && v<r) candLSet.insert(v);
        for (int candL : candLSet) {
            lo[data.m] = hi[data.m] = (double)candL;
            if (!tree.pointExistsInRange(lo, hi)) continue;
            updateResult(candL, r, qL, qR, alpha, beta, bestT, bestL, bestR);
        }
    };

    if (qR <= n - qL) {

        // ---- LEFT SIDE smaller ----

        // Phase 1: L goes LEFT from qL toward 0 with pruning
        for (int p = qL-2; p >= 0; --p) {
            int L = p+1;
            if (upperBoundLeftPhase1(L, qL, qR, beta) <= bestT) break;
            tryLeft(p);
        }

        // Phase 2: L goes RIGHT from qL toward qR with pruning
        for (int p = qL-1; p <= qR-1; ++p) {
            int L = p+1;
            if (upperBoundLeftPhase2(L, qL, qR, alpha) <= bestT) break;
            tryLeft(p);
        }

    } else {

        // ---- RIGHT SIDE smaller ----

        // Phase 1: R goes RIGHT from qR toward n with pruning
        for (int r = qR; r <= n; ++r) {
            if (upperBoundRightPhase1(r, qL, qR, beta) <= bestT) break;
            tryRight(r);
        }

        // Phase 2: R goes LEFT from qR toward qL with pruning
        for (int r = qR-1; r >= qL; --r) {
            if (upperBoundRightPhase2(r, qL, qR, alpha) <= bestT) break;
            tryRight(r);
        }
    }

    if (bestL==-1) return nullopt;
    return Result{data.keys[bestL-1], data.keys[bestR-1], bestT};
}
