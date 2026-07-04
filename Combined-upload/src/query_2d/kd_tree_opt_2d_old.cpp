#include "kd_tree_opt_2d_old.h"
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

optional<Result2D> queryBestKD_opt_2d_old(
    RawData2D &raw,
    double qX_min, double qX_max,
    double qY_min, double qY_max,
    double alpha, double beta) {

    int n_total = raw.pts.size();
    if (n_total == 0) return nullopt;

    // ============================================================
    // STEP 1: Sort all points by X (0-indexed)
    // ============================================================
    vector<RawPoint> sorted_pts_X = raw.pts;
    sort(sorted_pts_X.begin(), sorted_pts_X.end(),
         [](const RawPoint& a, const RawPoint& b){ return a.x < b.x; });

    // Prefix sum of query points in X-sorted order
    // prefix_Q[i] = number of query points in positions 0..i-1
    vector<int> prefix_Q(n_total + 1, 0);
    for (int i = 0; i < n_total; ++i) {
        prefix_Q[i+1] = prefix_Q[i];
        if (sorted_pts_X[i].x >= qX_min && sorted_pts_X[i].x <= qX_max &&
            sorted_pts_X[i].y >= qY_min && sorted_pts_X[i].y <= qY_max) {
            prefix_Q[i+1]++;
        }
    }
    int count_O = prefix_Q[n_total];
    if (count_O == 0) return nullopt;

    // Find qL_X, qR_X (0-indexed): first and last X-sorted positions
    // with X coordinate inside the query X range
    int qL_X = 0, qR_X = n_total - 1;
    while (qL_X < n_total && sorted_pts_X[qL_X].x < qX_min) qL_X++;
    while (qR_X >= 0 && sorted_pts_X[qR_X].x > qX_max) qR_X--;
    if (qL_X > qR_X) return nullopt;

    double bestT = -1.0;
    Result2D bestRes = {0, 0, 0, 0, -1.0};

    // ============================================================
    // STEP 2: Upper bound functions for X pruning
    // (Tversky similarity bounds — no KD-tree needed for X)
    // ============================================================

    // UB1: Based on points outside query X range included in I (increases |I\O|)
    // As X_L moves left of qL_X or X_R moves right of qR_X, more non-query points
    // are included, increasing |I\O| and decreasing similarity.
    auto getUB1 = [&](int X_L, int X_R) -> double {
        int A_out = max(0, qL_X - X_L) + max(0, X_R - qR_X);
        if (A_out <= 0) return 1.0;
        return (double)count_O / (count_O + alpha * A_out);
    };

    // UB2: Based on query points remaining in X range (limits intersection)
    // As X_L moves right or X_R moves left, fewer query points are in range,
    // reducing the maximum possible intersection.
    auto getUB2 = [&](int X_L, int X_R) -> double {
        if (X_L > X_R) return 0.0;
        int Q_rem = prefix_Q[X_R + 1] - prefix_Q[X_L];
        if (Q_rem <= 0) return 0.0;
        return (double)Q_rem / (Q_rem + beta * (count_O - Q_rem));
    };

    auto getUB = [&](int X_L, int X_R) -> double {
        return min(getUB1(X_L, X_R), getUB2(X_L, X_R));
    };

    // ============================================================
    // STEP 3: evaluateXRange — for a given X range [X_L, X_R] (0-indexed),
    // filter points to that X-slice, build KD-tree on Y, run full _opt on Y
    // (inner loop uses KD-tree, identical to 1D kd_tree_opt.cpp)
    // ============================================================
    auto evaluateXRange = [&](int X_L, int X_R) {
        // Extract subset of points within this X range, sort by Y
        vector<RawPoint> subY;
        subY.reserve(X_R - X_L + 1);
        for (int i = X_L; i <= X_R; ++i) {
            subY.push_back(sorted_pts_X[i]);
        }
        sort(subY.begin(), subY.end(),
             [](const RawPoint& a, const RawPoint& b){ return a.y < b.y; });

        int nY = subY.size();
        if (nY == 0) return;

        // Build prefix array for intersection counting within this Y-sorted subset
        vector<int> prefix_interY(nY + 1, 0);
        for (int i = 0; i < nY; ++i) {
            prefix_interY[i+1] = prefix_interY[i];
            if (subY[i].x >= qX_min && subY[i].x <= qX_max &&
                subY[i].y >= qY_min && subY[i].y <= qY_max) {
                prefix_interY[i+1]++;
            }
        }
        if (prefix_interY[nY] == 0) return; // No query points in this X-slice

        // Build Data and KD-tree on Y (inner dimension)
        Data dataY = buildSubsetData(subY, raw);
        KDTree treeY(dataY.points, dataY.m + 1);

        // Find qL_Y, qR_Y (1-indexed, matching 1D _opt convention)
        int L0_Y = 0, R0_Y = nY - 1;
        while (L0_Y < nY && subY[L0_Y].y < qY_min) L0_Y++;
        while (R0_Y >= 0 && subY[R0_Y].y > qY_max) R0_Y--;
        L0_Y = max(L0_Y, 0); R0_Y = min(R0_Y, nY - 1);
        if (L0_Y > R0_Y) return;

        int qL_Y = L0_Y + 1, qR_Y = R0_Y + 1;  // 1-indexed

        double x_min_range = sorted_pts_X[X_L].x;
        double x_max_range = sorted_pts_X[X_R].x;

        vector<double> lo(dataY.m + 1), hi(dataY.m + 1);

        // tryLeftY: fix left Y boundary at point p (0-indexed into dataY.points),
        // use KD-tree to find matching right Y boundary
        // (identical to tryLeft in 1D kd_tree_opt.cpp)
        auto tryLeftY = [&](int p) {
            const auto &center = dataY.points[p];
            setQueryBounds(dataY, center, lo, hi);
            lo[dataY.m] = p + 1; hi[dataY.m] = nY;
            auto [bestR_1, bestR_2] = getBothClosest(treeY, lo, hi, qR_Y, dataY.m);
            for (int bR : {bestR_1, bestR_2}) {
                if (bR != -1) {
                    int count_I = bR - (p + 1) + 1;
                    int count_inter = prefix_interY[bR] - prefix_interY[p];
                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);
                    if (t > bestT) {
                        bestT = t;
                        bestRes = {x_min_range, x_max_range, subY[p].y, subY[bR-1].y, t};
                    }
                }
            }
        };

        // tryRightY: fix right Y boundary at point r (1-indexed into dataY.points),
        // use KD-tree to find matching left Y boundary
        // (identical to tryRight in 1D kd_tree_opt.cpp)
        auto tryRightY = [&](int r) {
            const auto &center = dataY.points[r];
            setQueryBounds(dataY, center, lo, hi, true);
            lo[dataY.m] = 0; hi[dataY.m] = r - 1;
            auto [bestL_1, bestL_2] = getBothClosest(treeY, lo, hi, qL_Y - 1, dataY.m);
            for (int bL : {bestL_1, bestL_2}) {
                if (bL != -1) {
                    int L = bL + 1;
                    int count_I = r - L + 1;
                    int count_inter = prefix_interY[r] - prefix_interY[L - 1];
                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);
                    if (t > bestT) {
                        bestT = t;
                        bestRes = {x_min_range, x_max_range, subY[L-1].y, subY[r-1].y, t};
                    }
                }
            }
        };

        int local_count_O = prefix_interY[nY]; // Query points in THIS X-slice only

        // Run full _opt on Y (all 4 phases, identical to 1D kd_tree_opt.cpp)
        if (qR_Y <= nY - qL_Y) {
            // ---- LEFT SIDE smaller ----

            // Phase 1: L goes LEFT from qL toward 0 (expanding outward)
            for (int p = qL_Y - 2; p >= 0; --p) {
                int L = p + 1;
                double ub = (double)local_count_O / (local_count_O + alpha * (qL_Y - L));
                if (ub <= bestT) break;
                tryLeftY(p);
            }

            // Phase 2: L goes RIGHT from qL toward qR (contracting inward)
            for (int p = qL_Y - 1; p <= qR_Y - 1; ++p) {
                int L = p + 1;
                int Q_rem = prefix_interY[nY] - prefix_interY[L - 1];
                if (Q_rem <= 0) break;
                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));
                if (ub <= bestT) break;
                tryLeftY(p);
            }
        } else {
            // ---- RIGHT SIDE smaller ----

            // Phase 1: R goes RIGHT from qR toward n (expanding outward)
            for (int r = qR_Y; r <= nY; ++r) {
                double ub = (double)local_count_O / (local_count_O + alpha * (r - qR_Y));
                if (ub <= bestT) break;
                tryRightY(r);
            }

            // Phase 2: R goes LEFT from qR toward qL (contracting inward)
            for (int r = qR_Y - 1; r >= qL_Y; --r) {
                int Q_rem = prefix_interY[r];
                if (Q_rem <= 0) break;
                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));
                if (ub <= bestT) break;
                tryRightY(r);
            }
        }
    };

    // ============================================================
    // STEP 4: Outer X loop — _opt-style pruning on X coordinates
    // Iterates X_L and X_R with Phase 1 (expand) / Phase 2 (contract)
    // pruning bounds. NO KD-tree on X — fairness depends on the 2D
    // subset which is only known after Y filtering.
    // ============================================================

    // Phase 1 for X_L: expand outward (X_L starts at qL_X, then goes left)
    // Starting from qL_X ensures all X_R variations are explored at qL_X too
    for (int X_L = qL_X; X_L >= 0; --X_L) {
        if (X_L < qL_X && getUB1(X_L, qR_X) <= bestT) break;  // Outer pruning

        // For each X_L, iterate X_R
        // X_R Phase 1: expand right
        for (int X_R = qR_X; X_R < n_total; ++X_R) {
            if (getUB(X_L, X_R) <= bestT) break;
            evaluateXRange(X_L, X_R);
        }
        // X_R Phase 2: contract left
        for (int X_R = qR_X - 1; X_R >= X_L; --X_R) {
            if (getUB(X_L, X_R) <= bestT) break;
            evaluateXRange(X_L, X_R);
        }
    }

    // Phase 2 for X_L: contract inward (X_L goes right of qL_X)
    for (int X_L = qL_X + 1; X_L <= qR_X; ++X_L) {
        if (getUB2(X_L, n_total - 1) <= bestT) break;  // Outer pruning

        // For each X_L, iterate X_R
        // X_R Phase 1: expand right
        for (int X_R = max(X_L, qR_X); X_R < n_total; ++X_R) {
            if (getUB(X_L, X_R) <= bestT) break;
            evaluateXRange(X_L, X_R);
        }
        // X_R Phase 2: contract left
        for (int X_R = min(n_total - 1, qR_X - 1); X_R >= X_L; --X_R) {
            if (getUB(X_L, X_R) <= bestT) break;
            evaluateXRange(X_L, X_R);
        }
    }

    if (bestT < 0) return nullopt;
    return bestRes;
}
