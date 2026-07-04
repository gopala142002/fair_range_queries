#include "kd_tree_opt_2d.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

// Computes Tversky similarity between candidate rectangle I
// and original query rectangle O.
static double tversky2D(int count_I, int count_O, int count_inter, double alpha, double beta) 
{
    double inter = count_inter;
    double onlyA = count_I - inter;   // Points in I but not in O.
    double onlyB = count_O - inter;   // Points in O but not in I.
    double denom = inter + alpha * onlyA + beta * onlyB;
    return denom == 0.0 ? 0.0 : inter / denom;
}

// Finds the fair 2D rectangle having the best Tversky similarity.
//
// Main idea:
// 1. Sort points globally by X.
// 2. Enumerate promising X-ranges.
// 3. Prune X-ranges using Tversky upper bounds.
// 4. For every surviving X-range, sort its points by Y.
// 5. Reduce the Y search region using direct fairness checks.
// 6. Build a KD-tree on fairness prefix points.
// 7. Search for suitable fair Y intervals.
optional<Result2D> queryBestKD_opt_2d(
    RawData2D &raw,
    double qX_min, double qX_max,
    double qY_min, double qY_max,
    double alpha, double beta) {

    // Total number of points.
    int n_total = raw.pts.size();

    if (n_total == 0) 
        return nullopt;

    // Sort all points by X because the outer search enumerates X-ranges.
    vector<RawPoint> sorted_pts_X = raw.pts;
    sort(sorted_pts_X.begin(), sorted_pts_X.end(),[](const RawPoint& a, const RawPoint& b){ return a.x < b.x; });

    // prefix_Q[k] stores the number of original-query points
    // among sorted_pts_X[0 ... k-1].
    vector<int> prefix_Q(n_total + 1, 0);
    for (int i = 0; i < n_total; ++i) 
    {
        prefix_Q[i+1] = prefix_Q[i];

        // Increase prefix count if point belongs to original query O.
        if (sorted_pts_X[i].x >= qX_min && sorted_pts_X[i].x <= qX_max &&sorted_pts_X[i].y >= qY_min && sorted_pts_X[i].y <= qY_max) 
        {
            prefix_Q[i+1]++;
        }
    }

    // Total number of points inside original query rectangle O.
    int count_O = prefix_Q[n_total];

    if (count_O == 0) 
        return nullopt;

    // Find X-index range corresponding to original query X interval.
    int qL_X = 0, qR_X = n_total - 1;

    // First point with x >= qX_min.
    while (qL_X < n_total && sorted_pts_X[qL_X].x < qX_min) 
        qL_X++;

    // Last point with x <= qX_max.
    while (qR_X >= 0 && sorted_pts_X[qR_X].x > qX_max) 
        qR_X--;

    if (qL_X > qR_X) 
        return nullopt;

    // Best similarity and rectangle found so far.
    double bestT = -1.0;
    Result2D bestRes = {0, 0, 0, 0, -1.0};

    // UB1 handles expansion outside the original query X-range.
    //
    // A_out counts extra positions added outside [qL_X, qR_X].
    // These extra points contribute to the alpha penalty.
    auto getUB1 = [&](int X_L, int X_R) -> double 
    {
        int A_out = max(0, qL_X - X_L) + max(0, X_R - qR_X);

        if (A_out <= 0) 
            return 1.0;

        return (double)count_O / (count_O + alpha * A_out);
    };

    // UB2 handles removal of original-query points.
    //
    // Q_rem is the number of query points remaining in [X_L, X_R].
    auto getUB2 = [&](int X_L, int X_R) -> double 
    {
        if (X_L > X_R) 
            return 0.0;

        int Q_rem = prefix_Q[X_R + 1] - prefix_Q[X_L];

        if (Q_rem <= 0) 
            return 0.0;

        return (double)Q_rem / (Q_rem + beta * (count_O - Q_rem));
    };

    // Combined optimistic upper bound.
    auto getUB = [&](int X_L, int X_R) -> double 
    {
        return min(getUB1(X_L, X_R), getUB2(X_L, X_R));
    };

    // Evaluates one fixed X-range [X_L, X_R].
    // Inside this X-slab, the algorithm searches for a fair Y-range.


    auto evaluateXRange = [&](int X_L, int X_R) 
    {
        // Collect points belonging to current X-slab.
        vector<RawPoint> subY;
        subY.reserve(X_R - X_L + 1);

        for (int i = X_L; i <= X_R; ++i) 
        {
            subY.push_back(sorted_pts_X[i]);
        }

        // Sort slab points by Y for Y-interval searching.
        sort(subY.begin(), subY.end(),[](const RawPoint& a, const RawPoint& b){ return a.y < b.y; });

        int nY = subY.size();

        if (nY == 0) 
            return;

        // Find original query Y interval inside this Y-sorted slab.
        int L0_Y = 0, R0_Y = nY - 1;

        while (L0_Y < nY && subY[L0_Y].y < qY_min) 
            L0_Y++;

        while (R0_Y >= 0 && subY[R0_Y].y > qY_max) 
            R0_Y--;

        L0_Y = max(L0_Y, 0); 
        R0_Y = min(R0_Y, nY - 1);

        if (L0_Y > R0_Y) 
            return;

        // Save original query Y interval positions.
        int Ystart = L0_Y;
        int Yend = R0_Y;

        // Directly checks whether given color counts satisfy
        // all difference and ratio fairness constraints.
        auto checkCountsFair = [&](const std::unordered_map<std::string, int>& counts) 
        {
            // Check difference-based constraints.
            for (const auto& config : diffPairs) 
            {
                int cA = counts.count(config.colorA) ? counts.at(config.colorA) : 0;
                int cB = counts.count(config.colorB) ? counts.at(config.colorB) : 0;

                if (std::abs(config.weightA * cA - config.weightB * cB) > config.epsilon) 
                    return false;
            }

            // Check ratio-based constraints.
            for (const auto& config : ratioPairs) 
            {
                int cA = counts.count(config.colorA) ? counts.at(config.colorA) : 0;
                int cB = counts.count(config.colorB) ? counts.at(config.colorB) : 0;

                // Upper ratio condition.
                if (config.weightA * cA - config.weightB * cB * (1.0 + config.delta) > 0) 
                    return false;

                // Lower ratio condition.
                if (config.weightB * cB * (1.0 - config.delta) - config.weightA * cA > 0) 
                    return false;
            }

            return true;
        };

        // Expand query Y interval toward the right until fairness
        // is reached or the end of the slab is reached.
        int Yend_prime = Yend;
        std::unordered_map<std::string, int> countsR;

        // Initial color counts inside original query Y interval.
        for (int i = Ystart; i <= Yend; ++i) countsR[subY[i].color]++;
        
        while (Yend_prime < nY && !checkCountsFair(countsR)) 
        {
            Yend_prime++;

            if (Yend_prime < nY) 
            {
                countsR[subY[Yend_prime].color]++;
            }
        }

        if (Yend_prime >= nY) 
            Yend_prime = nY - 1;

        // Expand query Y interval toward the left until fairness
        // is reached or the beginning of the slab is reached.
        int Ystart_prime = Ystart;
        std::unordered_map<std::string, int> countsL;

        // Initial color counts inside original query Y interval.
        for (int i = Ystart; i <= Yend; ++i) countsL[subY[i].color]++;
        
        while (Ystart_prime >= 0 && !checkCountsFair(countsL)) 
        {
            Ystart_prime--;

            if (Ystart_prime >= 0) 
            {
                countsL[subY[Ystart_prime].color]++;
            }
        }

        if (Ystart_prime < 0) 
            Ystart_prime = 0;

        // Reduced Y search region is invalid.
        if (Ystart_prime > Yend_prime) 
            return; 

        // Keep only the reduced Y search region.
        std::vector<RawPoint> subY_prime;
        subY_prime.reserve(Yend_prime - Ystart_prime + 1);

        for (int i = Ystart_prime; i <= Yend_prime; ++i) 
        {
            subY_prime.push_back(subY[i]);
        }

        subY = std::move(subY_prime);
        nY = subY.size();

        // Convert original query Y indices into indices
        // relative to the reduced subY array.
        L0_Y = Ystart - Ystart_prime;
        R0_Y = Yend - Ystart_prime;

        // Convert to 1-based prefix interval positions.
        int qL_Y = L0_Y + 1, qR_Y = R0_Y + 1; 

        // prefix_interY[k] stores number of original-query points
        // among subY[0 ... k-1].
        vector<int> prefix_interY(nY + 1, 0);

        for (int i = 0; i < nY; ++i) 
        {
            prefix_interY[i+1] = prefix_interY[i];

            if (subY[i].x >= qX_min && subY[i].x <= qX_max && subY[i].y >= qY_min && subY[i].y <= qY_max) 
            {
                prefix_interY[i+1]++;
            }
        }

        // No original-query point exists in this reduced region.
        if (prefix_interY[nY] == 0) 
            return; 

        // Build transformed fairness prefix points.
        Data dataY = buildSubsetData(subY, raw);

        // Build KD-tree over fairness dimensions plus prefix index.
        KDTree treeY(dataY.points, dataY.m + 1);

        // Actual X boundaries of current fixed X-slab.
        double x_min_range = sorted_pts_X[X_L].x;
        double x_max_range = sorted_pts_X[X_R].x;

        // Lower and upper bounds used for KD-tree range queries.
        vector<double> lo(dataY.m + 1), hi(dataY.m + 1);

        // Fix left prefix p and search for suitable right endpoint bR.
        auto tryLeftY = [&](int p) 
        {
            const auto &center = dataY.points[p];

            // Create fairness-compatible search bounds.
            setQueryBounds(dataY, center, lo, hi);

            // Right endpoint must be after p.
            lo[dataY.m] = p + 1; 
            hi[dataY.m] = nY;

            // Find feasible endpoints closest to original query right boundary.
            auto [bestR_1, bestR_2] = getBothClosest(treeY, lo, hi, qR_Y, dataY.m);

            for (int bR : {bestR_1, bestR_2}) 
            {
                if (bR != -1) 
                {
                    // Candidate interval is [p+1, bR].
                    int count_I = bR - (p + 1) + 1;

                    // Intersection with original query rectangle.
                    int count_inter = prefix_interY[bR] - prefix_interY[p];

                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);

                    if (t > bestT) 
                    {
                        bestT = t;
                        bestRes = {x_min_range, x_max_range, subY[p].y, subY[bR-1].y, t};
                    }
                }
            }
        };

        // Fix right prefix r and search for suitable left endpoint bL.
        auto tryRightY = [&](int r) 
        {
            const auto &center = dataY.points[r];

            // Reverse fairness bounds because we search backward.
            setQueryBounds(dataY, center, lo, hi, true);

            // Left prefix must occur before r.
            lo[dataY.m] = 0; 
            hi[dataY.m] = r - 1;

            // Find feasible left endpoints closest to query left boundary.
            auto [bestL_1, bestL_2] = getBothClosest(treeY, lo, hi, qL_Y - 1, dataY.m);

            for (int bL : {bestL_1, bestL_2}) 
            {
                if (bL != -1) 
                {
                    // Convert prefix index into interval start.
                    int L = bL + 1;

                    // Number of points in candidate interval [L, r].
                    int count_I = r - L + 1;

                    // Intersection count with original query.
                    int count_inter = prefix_interY[r] - prefix_interY[L - 1];

                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);

                    if (t > bestT) 
                    {
                        bestT = t;
                        bestRes = {x_min_range, x_max_range, subY[L-1].y, subY[r-1].y, t};
                    }
                }
            }
        };

        // Number of original-query points in current reduced Y region.
        int local_count_O = prefix_interY[nY]; 
      
        // Choose the cheaper side for endpoint enumeration.
        if (qR_Y <= nY - qL_Y) 
        {
            // Expand lower Y boundary below original query.
            // Extra candidate points contribute to alpha penalty.
            for (int p = qL_Y - 2; p >= 0; --p) 
            {
                int L = p + 1;

                double ub = (double)local_count_O / (local_count_O + alpha * (qL_Y - L));

                if (ub <= bestT) 
                    break;

                tryLeftY(p);
            }

            // Move lower Y boundary inside original query.
            // Removed query points contribute to beta penalty.
            for (int p = qL_Y - 1; p <= qR_Y - 1; ++p) 
            {
                int L = p + 1;

                int Q_rem = prefix_interY[nY] - prefix_interY[L - 1];

                if (Q_rem <= 0) 
                    break;

                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));

                if (ub <= bestT) 
                    break;

                tryLeftY(p);
            }
        } 
        else 
        {
            // Expand upper Y boundary above original query.
            // Added points contribute to alpha penalty.
            for (int r = qR_Y; r <= nY; ++r) 
            {
                double ub = (double)local_count_O / (local_count_O + alpha * (r - qR_Y));

                if (ub <= bestT) 
                    break;

                tryRightY(r);
            }

            // Move upper Y boundary inside original query.
            // Removed query points contribute to beta penalty.
            for (int r = qR_Y - 1; r >= qL_Y; --r) 
            {
                int Q_rem = prefix_interY[r];

                if (Q_rem <= 0) 
                    break;

                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));

                if (ub <= bestT) 
                    break;

                tryRightY(r);
            }
        }
    };



    // ---------------------------------------------------------
    // Candidate-based X boundary search.
    //
    // Instead of enumerating every possible X boundary, test
    // geometrically spaced boundaries around the original query.
    // This is an approximate diagnostic search.
    // ---------------------------------------------------------

    vector<int> leftCandidates;
    vector<int> rightCandidates;

    // Original query boundaries.
    leftCandidates.push_back(qL_X);
    rightCandidates.push_back(qR_X);

    // Generate geometrically increasing offsets.
    for (long long step = 1; step < n_total; step *= 2)
    {
        // Left boundary expansion and contraction.
        leftCandidates.push_back(
            max(0, qL_X - static_cast<int>(step))
        );

        leftCandidates.push_back(
            min(qR_X, qL_X + static_cast<int>(step))
        );

        // Right boundary expansion and contraction.
        rightCandidates.push_back(
            min(n_total - 1, qR_X + static_cast<int>(step))
        );

        rightCandidates.push_back(
            max(qL_X, qR_X - static_cast<int>(step))
        );
    }

    // Sort and remove duplicate candidate boundaries.
    sort(leftCandidates.begin(), leftCandidates.end());
    leftCandidates.erase(
        unique(leftCandidates.begin(), leftCandidates.end()),
        leftCandidates.end()
    );

    sort(rightCandidates.begin(), rightCandidates.end());
    rightCandidates.erase(
        unique(rightCandidates.begin(), rightCandidates.end()),
        rightCandidates.end()
    );

    // Evaluate the original query X-range first.
    // This provides an initial feasible score for upper-bound pruning.
    evaluateXRange(qL_X, qR_X);

    // Evaluate candidate X-range pairs.
    for (int X_L : leftCandidates)
    {
        for (int X_R : rightCandidates)
        {
            if (X_L > X_R)
                continue;

            if (getUB(X_L, X_R) <= bestT)
                continue;

            evaluateXRange(X_L, X_R);
        }
    }

    // No feasible fair rectangle found.
    if (bestT < 0) 
        return nullopt;
        
    return bestRes;
}