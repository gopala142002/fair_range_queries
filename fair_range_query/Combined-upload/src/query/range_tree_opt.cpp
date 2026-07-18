#include "range_tree_opt.h"

// Helper to update best result
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

// Tversky upper bound when L is left of qL, best possible R = qR
// T_upper = Q / (Q + beta*(qL-L))
static double upperBoundLeftPhase1(int L, int qL, int qR, double alpha) {
    double Q   = qR - qL + 1;
    double extra = qL - L;
    return Q / (Q + alpha * extra);
}

// Tversky upper bound when L is inside [qL,qR], best possible R = qR
// T_upper = (qR-L+1) / (qR-L+1 + alpha*(L-qL))
static double upperBoundLeftPhase2(int L, int qL, int qR, double beta) {
    double inter   = qR - L + 1;
    double onlyIn  = L - qL;
    return inter / (inter + beta * onlyIn);
}

// Tversky upper bound when R is right of qR, best possible L = qL
// T_upper = Q / (Q + beta*(R-qR))
static double upperBoundRightPhase1(int R, int qL, int qR, double alpha) {
    double Q     = qR - qL + 1;
    double extra = R - qR;
    return Q / (Q + alpha * extra);
}

// Tversky upper bound when R is inside [qL,qR], best possible L = qL
// T_upper = (R-qL+1) / (R-qL+1 + alpha*(qR-R))
static double upperBoundRightPhase2(int R, int qL, int qR, double beta) {
    double inter  = R - qL + 1;
    double onlyIn = qR - R;
    return inter / (inter + beta * onlyIn);
}

optional<Result> queryBestRT_opt(Data &data, RangeTree &tree,
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

    // Lambda: do fairness check and tree query for a fixed left boundary p
    // Returns best R for this p, updates bestT/bestL/bestR
    auto tryLeft = [&](int p) {
        const auto &center = data.points[p];
        setQueryBounds(data, center, lo, hi);
        lo[data.m]=p+1; hi[data.m]=n;
        auto [bestRforP_1, bestRforP_2] = getBothClosest(tree, lo, hi, qR, data.m);
        if (bestRforP_1 != -1) { int bestRforP = bestRforP_1; updateResult(p+1, bestRforP, qL, qR, alpha, beta, bestT, bestL, bestR, search_start_time); }
        if (bestRforP_2 != -1) { int bestRforP = bestRforP_2; updateResult(p+1, bestRforP, qL, qR, alpha, beta, bestT, bestL, bestR, search_start_time); }
    };

    // Lambda: do fairness check and tree query for a fixed right boundary r
    // Returns best L for this r, updates bestT/bestL/bestR
    auto tryRight = [&](int r) {
        const auto &center = data.points[r];
        setQueryBounds(data, center, lo, hi, true);
        lo[data.m]=0; hi[data.m]=r-1;    // L must be strictly left of r
        auto [bestLforR_1, bestLforR_2] = getBothClosest(tree, lo, hi, qL - 1, data.m);
        if (bestLforR_1 != -1) { int bestLforR = bestLforR_1 + 1; updateResult(bestLforR, r, qL, qR, alpha, beta, bestT, bestL, bestR, search_start_time); }
        if (bestLforR_2 != -1) { int bestLforR = bestLforR_2 + 1; updateResult(bestLforR, r, qL, qR, alpha, beta, bestT, bestL, bestR, search_start_time); }
    };

    // ================================================================
    // Decide which side is smaller
    // Left side:  fix L, loop from 0 to qR  → qR   iterations
    // Right side: fix R, loop from qL to n  → n-qL iterations
    // ================================================================

    if (qR <= n - qL) {

        // ---- LEFT SIDE is smaller ----

        // Phase 1: L goes LEFT from qL toward 0
        // Upper bound = Q / (Q + beta*(qL-L))
        // Prune when upper bound <= bestT
        for (int p = qL-2; p >= 0; --p) {
            // p is 0-based index, L = p+1, so L < qL means p < qL-1
            int L = p+1;
            double ub = upperBoundLeftPhase1(L, qL, qR, alpha);
            if (ub <= bestT) break;   // pruning — no point going further left
            tryLeft(p);
        }

        // Phase 2: L goes RIGHT from qL toward qR (L inside input range)
        // Upper bound = (qR-L+1) / (qR-L+1 + alpha*(L-qL))
        // Prune when upper bound <= bestT
        for (int p = qL-1; p <= qR-1; ++p) {
            int L = p+1;
            double ub = upperBoundLeftPhase2(L, qL, qR, beta);
            if (ub <= bestT) break;   // pruning
            tryLeft(p);
        }

    } else {

        // ---- RIGHT SIDE is smaller ----

        // Phase 1: R goes RIGHT from qR toward n
        // Upper bound = Q / (Q + beta*(R-qR))
        // Prune when upper bound <= bestT
        for (int r = qR; r <= n; ++r) {
            double ub = upperBoundRightPhase1(r, qL, qR, alpha);
            if (ub <= bestT) break;   // pruning
            tryRight(r);
        }

        // Phase 2: R goes LEFT from qR toward qL (R inside input range)
        // Upper bound = (R-qL+1) / (R-qL+1 + alpha*(qR-R))
        // Prune when upper bound <= bestT
        for (int r = qR-1; r >= qL; --r) {
            double ub = upperBoundRightPhase2(r, qL, qR, beta);
            if (ub <= bestT) break;   // pruning
            tryRight(r);
        }
    }

    if (bestL==-1) return nullopt;
    return Result{data.keys[bestL-1], data.keys[bestR-1], bestT};
}
