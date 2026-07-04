#include "kd_tree_opt_2d_old.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;


// Computes the Tversky similarity between:
//
// I = candidate rectangle point set
// O = original query rectangle point set
//
// count_I     = number of points inside candidate rectangle I
// count_O     = number of points inside original query rectangle O
// count_inter = number of points common to both I and O
//
// onlyA = |I - O|
// onlyB = |O - I|
//
// Tversky similarity:
//
// |I ∩ O|
// ------------------------------------------------
// |I ∩ O| + alpha|I - O| + beta|O - I|
static double tversky2D(int count_I, int count_O, int count_inter, double alpha, double beta) 
{
    double inter = count_inter;

    // Points inside candidate I but outside query O.
    double onlyA = count_I - inter;

    // Points inside query O but outside candidate I.
    double onlyB = count_O - inter;

    // Denominator of the Tversky similarity.
    double denom = inter + alpha * onlyA + beta * onlyB;

    // Avoid division by zero.
    return denom == 0.0 ? 0.0 : inter / denom;
}


// Optimized KD-tree-based search for the best fair 2D rectangle.
//
// Main idea:
//
// 1. Sort all points by x-coordinate.
// 2. Enumerate promising x-ranges.
// 3. Use Tversky upper bounds to prune bad x-ranges.
// 4. For every promising x-range:
//      a. Extract points inside the x-slab.
//      b. Sort them by y-coordinate.
//      c. Build fairness prefix-space points.
//      d. Build a KD-tree on those prefix points.
//      e. Search for fair y-intervals.
// 5. Compute the Tversky similarity of candidates.
// 6. Keep the best result found.
//
// The algorithm uses branch-and-bound style pruning:
//
// if upper_bound <= bestT,
// then that search direction cannot improve the current answer.
optional<Result2D> queryBestKD_opt_2d_old(
    RawData2D &raw,
    double qX_min, double qX_max,
    double qY_min, double qY_max,
    double alpha, double beta) {

    // Total number of points in the dataset.
    int n_total = raw.pts.size();


    // Empty dataset.
    if (n_total == 0) 
        return nullopt;


    // Copy all points and sort them by x-coordinate.
    vector<RawPoint> sorted_pts_X = raw.pts;

    sort(sorted_pts_X.begin(), sorted_pts_X.end(),[](const RawPoint& a, const RawPoint& b){ return a.x < b.x; });


    // Prefix array storing how many points of the original
    // query rectangle appear in the x-sorted order.
    //
    // prefix_Q[k] =
    // number of query points among sorted_pts_X[0 ... k-1].
    vector<int> prefix_Q(n_total + 1, 0);


    // Build the query-point prefix array.
    for (int i = 0; i < n_total; ++i) 
    {
        // Carry forward previous prefix value.
        prefix_Q[i+1] = prefix_Q[i];


        // If the current point lies inside the original
        // query rectangle, increment the prefix count.
        if (sorted_pts_X[i].x >= qX_min && sorted_pts_X[i].x <= qX_max && sorted_pts_X[i].y >= qY_min && sorted_pts_X[i].y <= qY_max) 
        {
            prefix_Q[i+1]++;
        }
    }


    // Total number of points in the original query rectangle O.
    int count_O = prefix_Q[n_total];


    // Original query rectangle contains no points.
    if (count_O == 0) 
        return nullopt;


    // Find the first point whose x-coordinate is inside
    // or to the right of qX_min.
    int qL_X = 0, qR_X = n_total - 1;

    while (qL_X < n_total && sorted_pts_X[qL_X].x < qX_min) qL_X++;


    // Find the last point whose x-coordinate is inside
    // or to the left of qX_max.
    while (qR_X >= 0 && sorted_pts_X[qR_X].x > qX_max) qR_X--;


    // No valid x-range overlaps the query.
    if (qL_X > qR_X) return nullopt;


    // Best Tversky similarity found so far.
    double bestT = -1.0;


    // Best rectangle found so far.
    Result2D bestRes = {0, 0, 0, 0, -1.0};


    // Upper bound UB1.
    //
    // This bound estimates the best possible similarity when
    // the candidate x-range includes extra points outside
    // the query x-range.
    //
    // A_out represents the number of x-sorted positions added
    // outside the query x-index interval.
    //
    // More extra candidate points increase |I - O|,
    // which reduces the maximum possible similarity.
    auto getUB1 = [&](int X_L, int X_R) -> double 
    {
        int A_out = max(0, qL_X - X_L) + max(0, X_R - qR_X);


        // If no extra positions are added,
        // the optimistic upper bound is 1.
        if (A_out <= 0) return 1.0;


        return (double)count_O / (count_O + alpha * A_out);
    };


    // Upper bound UB2.
    //
    // This bound handles the case where the candidate x-range
    // excludes some points belonging to the original query.
    //
    // Q_rem = number of original query points remaining
    // inside the current x-range.
    //
    // Missing query points contribute to |O - I|.
    auto getUB2 = [&](int X_L, int X_R) -> double 
    {
        // Invalid x-range.
        if (X_L > X_R) 
            return 0.0;


        // Count query points remaining in this x-range.
        int Q_rem = prefix_Q[X_R + 1] - prefix_Q[X_L];


        // Candidate range contains no query points.
        if (Q_rem <= 0) 
            return 0.0;


        return (double)Q_rem / (Q_rem + beta * (count_O - Q_rem));
    };


    // Combined upper bound.
    //
    // Both UB1 and UB2 must hold, therefore the tighter
    // optimistic bound is their minimum.
    auto getUB = [&](int X_L, int X_R) -> double 
    {
        return min(getUB1(X_L, X_R), getUB2(X_L, X_R));
    };

   
    // Evaluates one selected x-range [X_L, X_R].
    //
    // Inside this x-slab, the algorithm searches for
    // the best fair y-interval using a KD-tree.
    auto evaluateXRange = [&](int X_L, int X_R) 
    {
        // Points belonging to the current x-slab.
        vector<RawPoint> subY;


        // Reserve memory for the x-slab points.
        subY.reserve(X_R - X_L + 1);


        // Copy points from the selected x-range.
        for (int i = X_L; i <= X_R; ++i) 
        {
            subY.push_back(sorted_pts_X[i]);
        }


        // Sort the current x-slab by y-coordinate.
        sort(subY.begin(), subY.end(),[](const RawPoint& a, const RawPoint& b){ return a.y < b.y; });


        // Number of points in the current x-slab.
        int nY = subY.size();


        // Empty x-slab.
        if (nY == 0) 
            return;


        // Prefix array for intersection with the original query.
        //
        // prefix_interY[k] =
        // number of query points among subY[0 ... k-1].
        vector<int> prefix_interY(nY + 1, 0);


        // Build the intersection prefix array.
        for (int i = 0; i < nY; ++i) 
        {
            prefix_interY[i+1] = prefix_interY[i];


            // Check whether this point belongs to the
            // original query rectangle.
            if (subY[i].x >= qX_min && subY[i].x <= qX_max && subY[i].y >= qY_min && subY[i].y <= qY_max) 
            {
                prefix_interY[i+1]++;
            }
        }


        // Current x-slab contains no query points.
        if (prefix_interY[nY] == 0) 
            return;
       

        // Build fairness prefix-space data for the y-sorted subset.
        Data dataY = buildSubsetData(subY, raw);


        // Build KD-tree over transformed prefix points.
        //
        // Dimensions:
        //
        // dataY.m fairness dimensions
        // +
        // 1 prefix-position dimension
        KDTree treeY(dataY.points, dataY.m + 1);


        // Find the y-index range corresponding to
        // the original query y-interval.
        int L0_Y = 0, R0_Y = nY - 1;


        // Find first y-position >= qY_min.
        while (L0_Y < nY && subY[L0_Y].y < qY_min) 
            L0_Y++;


        // Find last y-position <= qY_max.
        while (R0_Y >= 0 && subY[R0_Y].y > qY_max) 
            R0_Y--;


        // Clamp indices to valid bounds.
        L0_Y = max(L0_Y, 0); 
        R0_Y = min(R0_Y, nY - 1);


        // Query y-range does not overlap this x-slab.
        if (L0_Y > R0_Y) 
            return;


        // Convert to 1-based prefix interval indices.
        int qL_Y = L0_Y + 1, qR_Y = R0_Y + 1;  


        // Actual x-boundaries of the current x-slab.
        double x_min_range = sorted_pts_X[X_L].x;
        double x_max_range = sorted_pts_X[X_R].x;


        // Lower and upper bounds for KD-tree range queries.
        vector<double> lo(dataY.m + 1), hi(dataY.m + 1);


        // Searches for a suitable right endpoint bR
        // when the left-side prefix point p is fixed.
        //
        // Candidate interval:
        //
        // [p + 1, bR]
        auto tryLeftY = [&](int p) 
        {
            // Prefix-space point representing the fixed left endpoint.
            const auto &center = dataY.points[p];


            // Build KD-tree fairness query bounds around this prefix point.
            setQueryBounds(dataY, center, lo, hi);


            // Right endpoint must come after p.
            lo[dataY.m] = p + 1; hi[dataY.m] = nY;


            // Find candidate right endpoints closest to qR_Y.
            //
            // Two candidates are returned because the closest valid
            // endpoint may lie on either side of the target position.
            auto [bestR_1, bestR_2] = getBothClosest(treeY, lo, hi, qR_Y, dataY.m);


            // Evaluate both returned candidates.
            for (int bR : {bestR_1, bestR_2}) 
            {
                if (bR != -1) 
                {
                    // Candidate interval size.
                    int count_I = bR - (p + 1) + 1;


                    // Number of points shared with the original query.
                    int count_inter = prefix_interY[bR] - prefix_interY[p];


                    // Compute Tversky similarity.
                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);


                    // Update the best result if improved.
                    if (t > bestT) 
                    {
                        bestT = t;

                        bestRes = {x_min_range, x_max_range, subY[p].y, subY[bR-1].y, t};
                    }
                }
            }
        };


        // Searches for a suitable left endpoint bL
        // when the right prefix endpoint r is fixed.
        //
        // Candidate interval:
        //
        // [bL + 1, r]
        auto tryRightY = [&](int r) 
        {
            // Prefix-space point representing the fixed right endpoint.
            const auto &center = dataY.points[r];


            // Build reversed fairness query bounds.
            //
            // The true argument indicates that the search direction
            // is reversed because we are searching for a left prefix.
            setQueryBounds(dataY, center, lo, hi, true);


            // Left prefix must occur before r.
            lo[dataY.m] = 0; hi[dataY.m] = r - 1;


            // Find candidate left endpoints closest to qL_Y - 1.
            auto [bestL_1, bestL_2] = getBothClosest(treeY, lo, hi, qL_Y - 1, dataY.m);


            // Evaluate both returned left endpoints.
            for (int bL : {bestL_1, bestL_2}) 
            {
                if (bL != -1) 
                {
                    // Convert prefix endpoint to interval start.
                    int L = bL + 1;


                    // Number of points in candidate interval [L, r].
                    int count_I = r - L + 1;


                    // Number of candidate points inside original query.
                    int count_inter = prefix_interY[r] - prefix_interY[L - 1];


                    // Compute Tversky similarity.
                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);


                    // Update best result if improved.
                    if (t > bestT) 
                    {
                        bestT = t;

                        bestRes = {x_min_range, x_max_range, subY[L-1].y, subY[r-1].y, t};
                    }
                }
            }
        };


        // Number of original-query points present in
        // the current x-slab.
        int local_count_O = prefix_interY[nY]; 

        
        // Choose which side of the y-range to enumerate.
        //
        // The intention is to reduce work by searching from
        // the side that appears cheaper.
        if (qR_Y <= nY - qL_Y) 
        {
           
            // Case 1:
            // Expand the lower y-boundary below the query interval.
            //
            // This adds extra candidate points, contributing
            // to the alpha penalty.
            for (int p = qL_Y - 2; p >= 0; --p) 
            {
                int L = p + 1;


                // Optimistic similarity bound for this expansion.
                double ub = (double)local_count_O / (local_count_O + alpha * (qL_Y - L));


                // No further candidate in this direction
                // can improve the current best result.
                if (ub <= bestT) 
                    break;


                // Search for a compatible right y-endpoint.
                tryLeftY(p);
            }

           
            // Case 2:
            // Move the lower y-boundary inside the query interval.
            //
            // This may remove original query points and therefore
            // contributes to the beta penalty.
            for (int p = qL_Y - 1; p <= qR_Y - 1; ++p) 
            {
                int L = p + 1;


                // Number of query points remaining after
                // choosing this left boundary.
                int Q_rem = prefix_interY[nY] - prefix_interY[L - 1];


                // No query points remain.
                if (Q_rem <= 0) 
                    break;


                // Optimistic similarity upper bound.
                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));


                // Prune this search direction.
                if (ub <= bestT) 
                    break;


                // Search for a compatible right endpoint.
                tryLeftY(p);
            }
        } 
        else 
        {
            
            // Case 3:
            // Expand the upper y-boundary above the query interval.
            //
            // Extra candidate points contribute to alpha penalty.
            for (int r = qR_Y; r <= nY; ++r) 
            {
                // Optimistic upper bound for this expansion.
                double ub = (double)local_count_O / (local_count_O + alpha * (r - qR_Y));


                // Prune if improvement is impossible.
                if (ub <= bestT) 
                    break;


                // Search for a compatible left endpoint.
                tryRightY(r);
            }

            
            // Case 4:
            // Move the upper y-boundary inside the query interval.
            //
            // This removes query points and contributes
            // to the beta penalty.
            for (int r = qR_Y - 1; r >= qL_Y; --r) 
            {
                // Number of query points remaining.
                int Q_rem = prefix_interY[r];


                // No query points remain.
                if (Q_rem <= 0) 
                    break;


                // Optimistic similarity upper bound.
                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));


                // Prune this search direction.
                if (ub <= bestT) 
                    break;


                // Search for a compatible left endpoint.
                tryRightY(r);
            }
        }
    };

  
    // First major x-search direction:
    //
    // Start from qL_X and move the left boundary outward
    // toward smaller x-indices.
    for (int X_L = qL_X; X_L >= 0; --X_L) 
    {
        // If expanding further left cannot improve bestT,
        // stop this outer search direction.
        if (X_L < qL_X && getUB1(X_L, qR_X) <= bestT) 
            break; 

        
        // Search right boundary from the query's right side outward.
        for (int X_R = qR_X; X_R < n_total; ++X_R) 
        {
            // Prune when the upper bound cannot beat bestT.
            if (getUB(X_L, X_R) <= bestT) 
                break;


            // Evaluate this x-range using the y-dimensional KD-tree search.
            evaluateXRange(X_L, X_R);
        }
        

        // Search right boundary inward from just before qR_X.
        for (int X_R = qR_X - 1; X_R >= X_L; --X_R) 
        {
            // Prune when the upper bound cannot beat bestT.
            if (getUB(X_L, X_R) <= bestT) 
                break;


            // Evaluate this x-range.
            evaluateXRange(X_L, X_R);
        }
    }

   
    // Second major x-search direction:
    //
    // Move the left boundary inside the original query x-range.
    for (int X_L = qL_X + 1; X_L <= qR_X; ++X_L) 
    {
        // If excluding more query points cannot improve bestT,
        // stop this direction.
        if (getUB2(X_L, n_total - 1) <= bestT) 
            break;  

        
        // Search right boundary from the query's right side outward.
        for (int X_R = max(X_L, qR_X); X_R < n_total; ++X_R) 
        {
            // Prune if the optimistic upper bound is not competitive.
            if (getUB(X_L, X_R) <= bestT) 
                break;


            // Evaluate current x-range.
            evaluateXRange(X_L, X_R);
        }
       

        // Search right boundary inward.
        for (int X_R = min(n_total - 1, qR_X - 1); X_R >= X_L; --X_R) 
        {
            // Prune if improvement is impossible.
            if (getUB(X_L, X_R) <= bestT) 
                break;


            // Evaluate current x-range.
            evaluateXRange(X_L, X_R);
        }
    }


    // No feasible fair rectangle was found.
    if (bestT < 0) 
        return nullopt;


    // Return the fair rectangle having the highest
    // Tversky similarity found by the search.
    return bestRes;
}