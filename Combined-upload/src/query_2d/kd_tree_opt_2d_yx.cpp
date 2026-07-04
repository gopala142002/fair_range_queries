#include "kd_tree_opt_2d_yx.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

static double tversky2D(int count_I, int count_O, int count_inter, double alpha, double beta) {
    double inter = count_inter;
    double onlyA = count_I - inter;
    double onlyB = count_O - inter;
    double denom = inter + alpha * onlyA + beta * onlyB;
    return denom == 0.0 ? 0.0 : inter / denom;
}

optional<Result2D> queryBestKD_opt_2d_yx(
    RawData2D &raw,
    double qX_min, double qX_max,
    double qY_min, double qY_max,
    double alpha, double beta) {

    int n_total = raw.pts.size();
    if (n_total == 0) return nullopt;

    // ============================================================
    // STEP 1: Sort all points by Y (0-indexed)
    // ============================================================
    vector<RawPoint> sorted_pts_Y = raw.pts;
    sort(sorted_pts_Y.begin(), sorted_pts_Y.end(),
         [](const RawPoint& a, const RawPoint& b){ return a.y < b.y; });

    // Prefix sum of query points in Y-sorted order
    // prefix_Q[i] = number of query points in positions 0..i-1
    vector<int> prefix_Q(n_total + 1, 0);
    for (int i = 0; i < n_total; ++i) {
        prefix_Q[i+1] = prefix_Q[i];
        if (sorted_pts_Y[i].y >= qY_min && sorted_pts_Y[i].y <= qY_max &&
            sorted_pts_Y[i].x >= qX_min && sorted_pts_Y[i].x <= qX_max) {
            prefix_Q[i+1]++;
        }
    }
    int count_O = prefix_Q[n_total];
    if (count_O == 0) return nullopt;

    // Find qL_Y, qR_Y (0-indexed): first and last Y-sorted positions
    // with Y coordinate inside the query Y range
    int qL_Y = 0, qR_Y = n_total - 1;
    while (qL_Y < n_total && sorted_pts_Y[qL_Y].y < qY_min) qL_Y++;
    while (qR_Y >= 0 && sorted_pts_Y[qR_Y].y > qY_max) qR_Y--;
    if (qL_Y > qR_Y) return nullopt;

    double bestT = -1.0;
    Result2D bestRes = {0, 0, 0, 0, -1.0};

    // ============================================================
    // STEP 2: Upper bound functions for Y pruning
    // (Tversky similarity bounds — no KD-tree needed for Y)
    // ============================================================

    // UB1: Based on points outside query Y range included in I (increases |I\O|)
    // As Y_L moves left of qL_Y or Y_R moves right of qR_Y, more non-query points
    // are included, increasing |I\O| and decreasing similarity.
    auto getUB1 = [&](int Y_L, int Y_R) -> double {
        int A_out = max(0, qL_Y - Y_L) + max(0, Y_R - qR_Y);
        if (A_out <= 0) return 1.0;
        return (double)count_O / (count_O + alpha * A_out);
    };

    // UB2: Based on query points remaining in Y range (limits intersection)
    // As Y_L moves right or Y_R moves left, fewer query points are in range,
    // reducing the maximum possible intersection.
    auto getUB2 = [&](int Y_L, int Y_R) -> double {
        if (Y_L > Y_R) return 0.0;
        int Q_rem = prefix_Q[Y_R + 1] - prefix_Q[Y_L];
        if (Q_rem <= 0) return 0.0;
        return (double)Q_rem / (Q_rem + beta * (count_O - Q_rem));
    };

    auto getUB = [&](int Y_L, int Y_R) -> double {
        return min(getUB1(Y_L, Y_R), getUB2(Y_L, Y_R));
    };

    // ============================================================
    // STEP 3: evaluateYRange — for a given Y range [Y_L, Y_R] (0-indexed),
    // filter points to that Y-slice, build KD-tree on X, run full _opt on X
    // (inner loop uses KD-tree, identical to 1D kd_tree_opt.cpp)
    // ============================================================
    auto evaluateYRange = [&](int Y_L, int Y_R) {
        // Extract subset of points within this Y range, sort by X
        vector<RawPoint> subX;
        subX.reserve(Y_R - Y_L + 1);
        for (int i = Y_L; i <= Y_R; ++i) {
            subX.push_back(sorted_pts_Y[i]);
        }
        sort(subX.begin(), subX.end(),
             [](const RawPoint& a, const RawPoint& b){ return a.x < b.x; });

        int nX = subX.size();
        if (nX == 0) return;

        // 1. Identify Initial Query Bounds
        int L0_X = 0, R0_X = nX - 1;
        while (L0_X < nX && subX[L0_X].x < qX_min) L0_X++;
        while (R0_X >= 0 && subX[R0_X].x > qX_max) R0_X--;
        L0_X = max(L0_X, 0); R0_X = min(R0_X, nX - 1);

        if (L0_X > R0_X) return;

        int Xstart = L0_X;
        int Xend = R0_X;

        // Helper function to check fairness on raw color counts
        auto checkCountsFair = [&](const std::unordered_map<std::string, int>& counts) {
            for (const auto& config : diffPairs) {
                int cA = counts.count(config.colorA) ? counts.at(config.colorA) : 0;
                int cB = counts.count(config.colorB) ? counts.at(config.colorB) : 0;
                if (std::abs(config.weightA * cA - config.weightB * cB) > config.epsilon) return false;
            }
            for (const auto& config : ratioPairs) {
                int cA = counts.count(config.colorA) ? counts.at(config.colorA) : 0;
                int cB = counts.count(config.colorB) ? counts.at(config.colorB) : 0;
                if (config.weightA * cA - config.weightB * cB * (1.0 + config.delta) > 0) return false;
                if (config.weightB * cB * (1.0 - config.delta) - config.weightA * cA > 0) return false;
            }
            return true;
        };

        // 2. Find Xend'
        int Xend_prime = Xend;
        std::unordered_map<std::string, int> countsR;
        for (int i = Xstart; i <= Xend; ++i) countsR[subX[i].color]++;
        
        while (Xend_prime < nX && !checkCountsFair(countsR)) {
            Xend_prime++;
            if (Xend_prime < nX) {
                countsR[subX[Xend_prime].color]++;
            }
        }
        if (Xend_prime >= nX) Xend_prime = nX - 1;

        // 3. Find Xstart'
        int Xstart_prime = Xstart;
        std::unordered_map<std::string, int> countsL;
        for (int i = Xstart; i <= Xend; ++i) countsL[subX[i].color]++;
        
        while (Xstart_prime >= 0 && !checkCountsFair(countsL)) {
            Xstart_prime--;
            if (Xstart_prime >= 0) {
                countsL[subX[Xstart_prime].color]++;
            }
        }
        if (Xstart_prime < 0) Xstart_prime = 0;

        // 4. Subset Reduction
        if (Xstart_prime > Xend_prime) return; 

        std::vector<RawPoint> subX_prime;
        subX_prime.reserve(Xend_prime - Xstart_prime + 1);
        for (int i = Xstart_prime; i <= Xend_prime; ++i) {
            subX_prime.push_back(subX[i]);
        }
        subX = std::move(subX_prime);
        nX = subX.size();
        
        // Update L0_X and R0_X relative to the newly sliced subX
        L0_X = Xstart - Xstart_prime;
        R0_X = Xend - Xstart_prime;

        int qL_X = L0_X + 1, qR_X = R0_X + 1;  // 1-indexed

        // Build prefix array for intersection counting within this newly sliced X-sorted subset
        vector<int> prefix_interX(nX + 1, 0);
        for (int i = 0; i < nX; ++i) {
            prefix_interX[i+1] = prefix_interX[i];
            if (subX[i].y >= qY_min && subX[i].y <= qY_max &&
                subX[i].x >= qX_min && subX[i].x <= qX_max) {
                prefix_interX[i+1]++;
            }
        }
        if (prefix_interX[nX] == 0) return; // No query points in this slice

        // Build Data and KD-tree on X (inner dimension) using only the pruned subset
        Data dataX = buildSubsetData(subX, raw);
        KDTree treeX(dataX.points, dataX.m + 1);

        double y_min_range = sorted_pts_Y[Y_L].y;
        double y_max_range = sorted_pts_Y[Y_R].y;

        vector<double> lo(dataX.m + 1), hi(dataX.m + 1);

        // tryLeftX: fix left X boundary at point p (0-indexed into dataX.points),
        // use KD-tree to find matching right X boundary
        // (identical to tryLeft in 1D kd_tree_opt.cpp)
        auto tryLeftX = [&](int p) {
            const auto &center = dataX.points[p];
            setQueryBounds(dataX, center, lo, hi);
            lo[dataX.m] = p + 1; hi[dataX.m] = nX;
            auto [bestR_1, bestR_2] = getBothClosest(treeX, lo, hi, qR_X, dataX.m);
            for (int bR : {bestR_1, bestR_2}) {
                if (bR != -1) {
                    int count_I = bR - (p + 1) + 1;
                    int count_inter = prefix_interX[bR] - prefix_interX[p];
                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);
                    if (t > bestT) {
                        bestT = t;
                        bestRes = {subX[p].x, subX[bR-1].x, y_min_range, y_max_range, t};
                    }
                }
            }
        };

        // tryRightX: fix right X boundary at point r (1-indexed into dataX.points),
        // use KD-tree to find matching left X boundary
        // (identical to tryRight in 1D kd_tree_opt.cpp)
        auto tryRightX = [&](int r) {
            const auto &center = dataX.points[r];
            setQueryBounds(dataX, center, lo, hi, true);
            lo[dataX.m] = 0; hi[dataX.m] = r - 1;
            auto [bestL_1, bestL_2] = getBothClosest(treeX, lo, hi, qL_X - 1, dataX.m);
            for (int bL : {bestL_1, bestL_2}) {
                if (bL != -1) {
                    int L = bL + 1;
                    int count_I = r - L + 1;
                    int count_inter = prefix_interX[r] - prefix_interX[L - 1];
                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);
                    if (t > bestT) {
                        bestT = t;
                        bestRes = {subX[L-1].x, subX[r-1].x, y_min_range, y_max_range, t};
                    }
                }
            }
        };

        int local_count_O = prefix_interX[nX]; // Query points in THIS Y-slice only

        // Run full _opt on X (all 4 phases, identical to 1D kd_tree_opt.cpp)
        if (qR_X <= nX - qL_X) {
            // ---- LEFT SIDE smaller ----

            // Phase 1: L goes LEFT from qL toward 0 (expanding outward)
            for (int p = qL_X - 2; p >= 0; --p) {
                int L = p + 1;
                double ub = (double)local_count_O / (local_count_O + alpha * (qL_X - L));
                if (ub <= bestT) break;
                tryLeftX(p);
            }

            // Phase 2: L goes RIGHT from qL toward qR (contracting inward)
            for (int p = qL_X - 1; p <= qR_X - 1; ++p) {
                int L = p + 1;
                int Q_rem = prefix_interX[nX] - prefix_interX[L - 1];
                if (Q_rem <= 0) break;
                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));
                if (ub <= bestT) break;
                tryLeftX(p);
            }
        } else {
            // ---- RIGHT SIDE smaller ----

            // Phase 1: R goes RIGHT from qR toward n (expanding outward)
            for (int r = qR_X; r <= nX; ++r) {
                double ub = (double)local_count_O / (local_count_O + alpha * (r - qR_X));
                if (ub <= bestT) break;
                tryRightX(r);
            }

            // Phase 2: R goes LEFT from qR toward qL (contracting inward)
            for (int r = qR_X - 1; r >= qL_X; --r) {
                int Q_rem = prefix_interX[r];
                if (Q_rem <= 0) break;
                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));
                if (ub <= bestT) break;
                tryRightX(r);
            }
        }
    };

    // ============================================================
    // STEP 4: Outer Y loop — _opt-style pruning on Y coordinates
    // Iterates Y_L and Y_R with Phase 1 (expand) / Phase 2 (contract)
    // pruning bounds. NO KD-tree on Y — fairness depends on the 2D
    // subset which is only known after X filtering.
    // ============================================================

    // Phase 1 for Y_L: expand outward (Y_L starts at qL_Y, then goes left)
    // Starting from qL_Y ensures all Y_R variations are explored at qL_Y too
    for (int Y_L = qL_Y; Y_L >= 0; --Y_L) {
        if (Y_L < qL_Y && getUB1(Y_L, qR_Y) <= bestT) break;  // Outer pruning

        // For each Y_L, iterate Y_R
        // Y_R Phase 1: expand right
        for (int Y_R = qR_Y; Y_R < n_total; ++Y_R) {
            if (getUB(Y_L, Y_R) <= bestT) break;
            evaluateYRange(Y_L, Y_R);
        }
        // Y_R Phase 2: contract left
        for (int Y_R = qR_Y - 1; Y_R >= Y_L; --Y_R) {
            if (getUB(Y_L, Y_R) <= bestT) break;
            evaluateYRange(Y_L, Y_R);
        }
    }

    // Phase 2 for Y_L: contract inward (Y_L goes right of qL_Y)
    for (int Y_L = qL_Y + 1; Y_L <= qR_Y; ++Y_L) {
        if (getUB2(Y_L, n_total - 1) <= bestT) break;  // Outer pruning

        // For each Y_L, iterate Y_R
        // Y_R Phase 1: expand right
        for (int Y_R = max(Y_L, qR_Y); Y_R < n_total; ++Y_R) {
            if (getUB(Y_L, Y_R) <= bestT) break;
            evaluateYRange(Y_L, Y_R);
        }
        // Y_R Phase 2: contract left
        for (int Y_R = min(n_total - 1, qR_Y - 1); Y_R >= Y_L; --Y_R) {
            if (getUB(Y_L, Y_R) <= bestT) break;
            evaluateYRange(Y_L, Y_R);
        }
    }

    if (bestT < 0) return nullopt;
    return bestRes;
}
