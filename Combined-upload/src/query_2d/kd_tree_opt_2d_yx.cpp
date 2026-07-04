#include "kd_tree_opt_2d_yx.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;


// Computes Tversky similarity between:
//
// I = candidate rectangle point set
// O = original query rectangle point set
//
// count_I     = number of points in candidate rectangle I
// count_O     = number of points in original query rectangle O
// count_inter = number of points common to I and O
//
// onlyA = |I - O|
// onlyB = |O - I|
static double tversky2D(int count_I, int count_O, int count_inter, double alpha, double beta) 
{
    double inter = count_inter;

    // Points present in candidate I but not in query O.
    double onlyA = count_I - inter;

    // Points present in query O but not in candidate I.
    double onlyB = count_O - inter;

    // Denominator of Tversky similarity.
    double denom = inter + alpha * onlyA + beta * onlyB;

    // Avoid division by zero.
    return denom == 0.0 ? 0.0 : inter / denom;
}


// Main optimized 2D KD-tree search.
//
// High-level idea:
//
// 1. Sort all points by Y.
// 2. Enumerate candidate Y-ranges.
// 3. Use upper bounds to prune bad Y-ranges.
// 4. For each surviving Y-range:
//      a. Collect points in the Y-slab.
//      b. Sort them by X.
//      c. Find the query X-range.
//      d. Extend the X search region toward possible fair regions.
//      e. Build prefix fairness representation.
//      f. Build a KD-tree.
//      g. Search for fair X intervals.
// 5. Keep the rectangle having maximum Tversky similarity.
optional<Result2D> queryBestKD_opt_2d_yx(
    RawData2D &raw,
    double qX_min, double qX_max,
    double qY_min, double qY_max,
    double alpha, double beta) {

    // Total number of points in the dataset.
    int n_total = raw.pts.size();


    // Empty dataset means no rectangle can be returned.
    if (n_total == 0) 
        return nullopt;


    // Copy all points.
    vector<RawPoint> sorted_pts_Y = raw.pts;


    // Sort all points by Y-coordinate.
    //
    // The outer search will enumerate Y-ranges.
    sort(sorted_pts_Y.begin(), sorted_pts_Y.end(),[](const RawPoint& a, const RawPoint& b){ return a.y < b.y; });


    // Prefix array for original-query membership.
    //
    // prefix_Q[k] =
    // number of original-query points among
    // sorted_pts_Y[0 ... k-1].
    vector<int> prefix_Q(n_total + 1, 0);


    // Build query-membership prefix array.
    for (int i = 0; i < n_total; ++i) 
    {
        // Carry forward the previous prefix value.
        prefix_Q[i+1] = prefix_Q[i];


        // Check whether this point belongs to
        // the original query rectangle O.
        if (sorted_pts_Y[i].y >= qY_min && sorted_pts_Y[i].y <= qY_max &&sorted_pts_Y[i].x >= qX_min && sorted_pts_Y[i].x <= qX_max) 
        {
            prefix_Q[i+1]++;
        }
    }


    // Total number of points in original query rectangle O.
    int count_O = prefix_Q[n_total];


    // Query rectangle contains no points.
    if (count_O == 0) 
        return nullopt;

  
    // Find the index range in Y-sorted order corresponding
    // to the original query Y interval.
    int qL_Y = 0, qR_Y = n_total - 1;


    // First point having y >= qY_min.
    while (qL_Y < n_total && sorted_pts_Y[qL_Y].y < qY_min) 
        qL_Y++;


    // Last point having y <= qY_max.
    while (qR_Y >= 0 && sorted_pts_Y[qR_Y].y > qY_max) 
        qR_Y--;


    // No valid query Y-range exists.
    if (qL_Y > qR_Y) 
        return nullopt;


    // Best similarity found so far.
    double bestT = -1.0;


    // Best rectangle found so far.
    Result2D bestRes = {0, 0, 0, 0, -1.0};

   
   
    // Upper bound UB1.
    //
    // Used when the candidate Y-range expands outside
    // the original query Y-range.
    //
    // A_out measures how many Y-sorted positions lie
    // outside the original query Y index interval.
    //
    // These extra positions can contribute to |I - O|,
    // producing an alpha penalty.
    auto getUB1 = [&](int Y_L, int Y_R) -> double 
    {
        // Number of extra positions outside query Y interval.
        int A_out = max(0, qL_Y - Y_L) + max(0, Y_R - qR_Y);


        // No outside expansion means the optimistic bound is 1.
        if (A_out <= 0) 
            return 1.0;


        // Optimistic Tversky upper bound.
        return (double)count_O / (count_O + alpha * A_out);
    };


    // Upper bound UB2.
    //
    // Used when the candidate Y-range removes some
    // points of the original query.
    //
    // Q_rem is the number of original-query points
    // remaining inside the selected Y-range.
    auto getUB2 = [&](int Y_L, int Y_R) -> double 
    {
        // Invalid Y-range.
        if (Y_L > Y_R) return 0.0;


        // Number of query points remaining in this Y-range.
        int Q_rem = prefix_Q[Y_R + 1] - prefix_Q[Y_L];


        // No query points remain.
        if (Q_rem <= 0) 
            return 0.0;


        // Missing query points contribute to beta penalty.
        return (double)Q_rem / (Q_rem + beta * (count_O - Q_rem));
    };


    // Combined upper bound.
    //
    // Both upper bounds must hold, so the tighter
    // bound is their minimum.
    auto getUB = [&](int Y_L, int Y_R) -> double 
    {
        return min(getUB1(Y_L, Y_R), getUB2(Y_L, Y_R));
    };

   
    
    // Evaluates one selected Y-range [Y_L, Y_R].
    //
    // For this fixed Y-slab, the algorithm searches
    // for the best fair X-range.
    auto evaluateYRange = [&](int Y_L, int Y_R) 
    {
        // Points belonging to the current Y-slab.
        vector<RawPoint> subX;


        // Reserve enough memory for all points in the slab.
        subX.reserve(Y_R - Y_L + 1);


        // Copy all points from the current Y-range.
        for (int i = Y_L; i <= Y_R; ++i) 
        {
            subX.push_back(sorted_pts_Y[i]);
        }


        // Sort the points of the current Y-slab by X-coordinate.
        //
        // Now contiguous intervals in subX represent X-ranges.
        sort(subX.begin(), subX.end(),[](const RawPoint& a, const RawPoint& b){ return a.x < b.x; });


        // Number of points in current Y-slab.
        int nX = subX.size();


        // Empty Y-slab.
        if (nX == 0) 
            return;


        // Find the X-index interval corresponding to
        // the original query X-range.
        int L0_X = 0, R0_X = nX - 1;


        // First point with x >= qX_min.
        while (L0_X < nX && subX[L0_X].x < qX_min) 
            L0_X++;


        // Last point with x <= qX_max.
        while (R0_X >= 0 && subX[R0_X].x > qX_max) 
            R0_X--;


        // Clamp indices to valid range.
        L0_X = max(L0_X, 0); 
        R0_X = min(R0_X, nX - 1);


        // No query X-range exists in this Y-slab.
        if (L0_X > R0_X) 
            return;


        // Save original query X interval positions.
        int Xstart = L0_X;
        int Xend = R0_X;



        // Helper function that checks fairness directly
        // from color counts.
        //
        // This is used before building the KD-tree to determine
        // how far the X search region may need to extend.
        auto checkCountsFair = [&](const std::unordered_map<std::string, int>& counts) 
        {
            // Check difference-based fairness constraints.
            for (const auto& config : diffPairs) 
            {
                // Count of color A.
                int cA = counts.count(config.colorA) ? counts.at(config.colorA) : 0;


                // Count of color B.
                int cB = counts.count(config.colorB) ? counts.at(config.colorB) : 0;


                // Difference constraint:
                //
                // |weightA*cA - weightB*cB| <= epsilon
                if (std::abs(config.weightA * cA - config.weightB * cB) > config.epsilon) 
                    return false;
            }


            // Check ratio-based fairness constraints.
            for (const auto& config : ratioPairs) 
            {
                // Count of color A.
                int cA = counts.count(config.colorA) ? counts.at(config.colorA) : 0;


                // Count of color B.
                int cB = counts.count(config.colorB) ? counts.at(config.colorB) : 0;


                // Upper ratio constraint.
                if (config.weightA * cA - config.weightB * cB * (1.0 + config.delta) > 0) 
                    return false;


                // Lower ratio constraint.
                if (config.weightB * cB * (1.0 - config.delta) - config.weightA * cA > 0) 
                    return false;
            }


            // All fairness constraints are satisfied.
            return true;
        };

  
        // Try expanding the original query X-range toward the right
        // until a fair color-count configuration is reached
        // or the end of subX is reached.
        int Xend_prime = Xend;


        // Color counts for right-side expansion.
        std::unordered_map<std::string, int> countsR;


        // Initialize counts using the original query X interval.
        for (int i = Xstart; i <= Xend; ++i) 
            countsR[subX[i].color]++;
        

        // Expand right while the current interval is unfair.
        while (Xend_prime < nX && !checkCountsFair(countsR)) 
        {
            Xend_prime++;


            // Add newly included point to color counts.
            if (Xend_prime < nX) 
            {
                countsR[subX[Xend_prime].color]++;
            }
        }


        // Clamp right expansion to valid index.
        if (Xend_prime >= nX) 
            Xend_prime = nX - 1;

      
        // Try expanding the original query X-range toward the left
        // until a fair color-count configuration is reached
        // or the beginning of subX is reached.
        int Xstart_prime = Xstart;


        // Color counts for left-side expansion.
        std::unordered_map<std::string, int> countsL;


        // Initialize counts with original query X interval.
        for (int i = Xstart; i <= Xend; ++i) 
            countsL[subX[i].color]++;
        

        // Expand left while current interval is unfair.
        while (Xstart_prime >= 0 && !checkCountsFair(countsL)) 
        {
            Xstart_prime--;


            // Add newly included point to color counts.
            if (Xstart_prime >= 0) 
            {
                countsL[subX[Xstart_prime].color]++;
            }
        }


        // Clamp left expansion to valid index.
        if (Xstart_prime < 0) 
            Xstart_prime = 0;


        // Invalid reduced search region.
        if (Xstart_prime > Xend_prime) 
            return; 


        // Create a smaller X-subset containing only the
        // region considered useful for the KD-tree search.
        std::vector<RawPoint> subX_prime;


        // Reserve memory for reduced X-subset.
        subX_prime.reserve(Xend_prime - Xstart_prime + 1);


        // Copy reduced search interval.
        for (int i = Xstart_prime; i <= Xend_prime; ++i) 
        {
            subX_prime.push_back(subX[i]);
        }


        // Replace original subX with reduced subX.
        subX = std::move(subX_prime);


        // Update number of X points.
        nX = subX.size();
        

        // Convert original query X boundaries into indices
        // relative to the reduced subX array.
        L0_X = Xstart - Xstart_prime;
        R0_X = Xend - Xstart_prime;


        // Convert to 1-based prefix interval positions.
        int qL_X = L0_X + 1, qR_X = R0_X + 1;  

      
        // Prefix array for counting intersection points.
        //
        // prefix_interX[k] =
        // number of original-query points among subX[0 ... k-1].
        vector<int> prefix_interX(nX + 1, 0);


        // Build intersection prefix array.
        for (int i = 0; i < nX; ++i) 
        {
            // Carry forward previous prefix value.
            prefix_interX[i+1] = prefix_interX[i];


            // Check whether this point belongs to
            // the original query rectangle.
            if (subX[i].y >= qY_min && subX[i].y <= qY_max && subX[i].x >= qX_min && subX[i].x <= qX_max) 
            {
                prefix_interX[i+1]++;
            }
        }


        // Reduced X-region contains no query points.
        if (prefix_interX[nX] == 0) 
            return; 

       
        // Build prefix fairness representation for X-sorted points.
        Data dataX = buildSubsetData(subX, raw);


        // Build KD-tree over transformed prefix-space points.
        //
        // Dimensions:
        //
        // dataX.m fairness dimensions
        // +
        // 1 prefix-index dimension
        KDTree treeX(dataX.points, dataX.m + 1);


        // Actual Y boundaries of the current fixed Y-slab.
        double y_min_range = sorted_pts_Y[Y_L].y;
        double y_max_range = sorted_pts_Y[Y_R].y;


        // KD-tree range-query bounds.
        vector<double> lo(dataX.m + 1), hi(dataX.m + 1);


        // Search for a suitable right X endpoint
        // when the left prefix endpoint p is fixed.
        //
        // Candidate X interval:
        //
        // [p + 1, bR]
        auto tryLeftX = [&](int p) 
        {
            // Prefix-space point corresponding to fixed left endpoint.
            const auto &center = dataX.points[p];


            // Construct fairness query bounds around center.
            setQueryBounds(dataX, center, lo, hi);


            // Right endpoint must occur after p.
            lo[dataX.m] = p + 1; hi[dataX.m] = nX;


            // Find valid right endpoints closest to qR_X.
            //
            // Two candidates are returned because the closest
            // feasible position may lie on either side of qR_X.
            auto [bestR_1, bestR_2] = getBothClosest(treeX, lo, hi, qR_X, dataX.m);


            // Evaluate both returned candidates.
            for (int bR : {bestR_1, bestR_2}) 
            {
                if (bR != -1) 
                {
                    // Number of points in candidate X interval.
                    int count_I = bR - (p + 1) + 1;


                    // Number of points shared with original query.
                    int count_inter = prefix_interX[bR] - prefix_interX[p];


                    // Compute Tversky similarity.
                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);


                    // Update best rectangle if similarity improves.
                    if (t > bestT) 
                    {
                        bestT = t;


                        // Rectangle:
                        //
                        // X boundaries come from KD-tree candidate interval.
                        // Y boundaries come from fixed Y-range.
                        bestRes = {subX[p].x, subX[bR-1].x, y_min_range, y_max_range, t};
                    }
                }
            }
        };

     
        // Search for a suitable left X endpoint
        // when the right prefix endpoint r is fixed.
        //
        // Candidate X interval:
        //
        // [bL + 1, r]
        auto tryRightX = [&](int r) 
        {
            // Prefix-space point corresponding to fixed right endpoint.
            const auto &center = dataX.points[r];


            // Build reversed fairness query bounds.
            //
            // true means we search backward for the left prefix.
            setQueryBounds(dataX, center, lo, hi, true);


            // Left prefix must occur before r.
            lo[dataX.m] = 0; hi[dataX.m] = r - 1;


            // Find feasible left endpoints closest to qL_X - 1.
            auto [bestL_1, bestL_2] = getBothClosest(treeX, lo, hi, qL_X - 1, dataX.m);


            // Evaluate both returned left endpoints.
            for (int bL : {bestL_1, bestL_2}) 
            {
                if (bL != -1) 
                {
                    // Convert prefix position into interval start.
                    int L = bL + 1;


                    // Number of points in candidate interval.
                    int count_I = r - L + 1;


                    // Number of candidate points also belonging
                    // to original query rectangle.
                    int count_inter = prefix_interX[r] - prefix_interX[L - 1];


                    // Compute Tversky similarity.
                    double t = tversky2D(count_I, count_O, count_inter, alpha, beta);


                    // Update best result if improved.
                    if (t > bestT) 
                    {
                        bestT = t;


                        // X boundaries come from candidate interval.
                        // Y boundaries come from fixed Y-slab.
                        bestRes = {subX[L-1].x, subX[r-1].x, y_min_range, y_max_range, t};
                    }
                }
            }
        };


        // Number of original-query points present
        // in the current reduced X search region.
        int local_count_O = prefix_interX[nX]; 



        // Choose which X endpoint to enumerate.
        //
        // The intention is to search from the cheaper side.
        if (qR_X <= nX - qL_X) 
        {
            // Expand the left X-boundary outside
            // the original query X-range.
            //
            // Extra points contribute to alpha penalty.
            for (int p = qL_X - 2; p >= 0; --p) 
            {
                // Candidate interval starts at p + 1.
                int L = p + 1;


                // Optimistic upper bound for left expansion.
                double ub = (double)local_count_O / (local_count_O + alpha * (qL_X - L));


                // Further expansion cannot improve bestT.
                if (ub <= bestT) 
                    break;


                // Use KD-tree to find compatible right endpoint.
                tryLeftX(p);
            }

          
            // Move the left X-boundary inside the query interval.
            //
            // Removing query points contributes to beta penalty.
            for (int p = qL_X - 1; p <= qR_X - 1; ++p) 
            {
                int L = p + 1;


                // Number of query points remaining after
                // choosing this left boundary.
                int Q_rem = prefix_interX[nX] - prefix_interX[L - 1];


                // No query points remain.
                if (Q_rem <= 0) 
                    break;


                // Optimistic upper bound.
                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));


                // Further movement cannot improve bestT.
                if (ub <= bestT) 
                    break;


                // Search for compatible right endpoint.
                tryLeftX(p);
            }
        } 
        else 
        {
           
            // Expand the right X-boundary outside
            // the original query X-range.
            //
            // Added points contribute to alpha penalty.
            for (int r = qR_X; r <= nX; ++r) 
            {
                // Optimistic upper bound for right expansion.
                double ub = (double)local_count_O / (local_count_O + alpha * (r - qR_X));


                // Further expansion cannot improve bestT.
                if (ub <= bestT) 
                    break;


                // Search for compatible left endpoint.
                tryRightX(r);
            }

    
            // Move the right X-boundary inside
            // the original query X-range.
            //
            // Removed query points contribute to beta penalty.
            for (int r = qR_X - 1; r >= qL_X; --r) 
            {
                // Number of query points remaining.
                int Q_rem = prefix_interX[r];


                // No query points remain.
                if (Q_rem <= 0) 
                    break;


                // Optimistic upper bound.
                double ub = (double)Q_rem / (Q_rem + beta * (local_count_O - Q_rem));


                // Further movement cannot improve bestT.
                if (ub <= bestT) 
                    break;


                // Search for compatible left endpoint.
                tryRightX(r);
            }
        }
    };


    // First major Y-search direction.
    //
    // Start from the original query's lower Y boundary
    // and move outward toward smaller Y indices.
    for (int Y_L = qL_Y; Y_L >= 0; --Y_L) 
    {
        // If expanding farther downward cannot improve bestT,
        // stop this entire direction.
        if (Y_L < qL_Y && getUB1(Y_L, qR_Y) <= bestT) 
            break; 


        // Search upper Y boundary outward.
        for (int Y_R = qR_Y; Y_R < n_total; ++Y_R) 
        {
            // Prune when upper bound cannot beat current best.
            if (getUB(Y_L, Y_R) <= bestT) 
                break;


            // Evaluate this Y-slab using X-dimensional KD-tree search.
            evaluateYRange(Y_L, Y_R);
        }


        // Search upper Y boundary inward.
        for (int Y_R = qR_Y - 1; Y_R >= Y_L; --Y_R) 
        {
            // Prune when improvement is impossible.
            if (getUB(Y_L, Y_R) <= bestT) 
                break;


            // Evaluate current Y-range.
            evaluateYRange(Y_L, Y_R);
        }
    }


    // Second major Y-search direction.
    //
    // Move the lower Y-boundary inside the original query Y-range.
    for (int Y_L = qL_Y + 1; Y_L <= qR_Y; ++Y_L) 
    {
        // If removing additional query points cannot improve bestT,
        // stop this direction.
        if (getUB2(Y_L, n_total - 1) <= bestT) 
            break;  


        // Search upper Y-boundary outward.
        for (int Y_R = max(Y_L, qR_Y); Y_R < n_total; ++Y_R) 
        {
            // Prune when upper bound cannot beat bestT.
            if (getUB(Y_L, Y_R) <= bestT) 
                break;


            // Evaluate current Y-slab.
            evaluateYRange(Y_L, Y_R);
        }
       

        // Search upper Y-boundary inward.
        for (int Y_R = min(n_total - 1, qR_Y - 1); Y_R >= Y_L; --Y_R) 
        {
            // Prune when improvement is impossible.
            if (getUB(Y_L, Y_R) <= bestT) 
                break;


            // Evaluate current Y-slab.
            evaluateYRange(Y_L, Y_R);
        }
    }


    // No fair candidate rectangle was found.
    if (bestT < 0) 
        return nullopt;


    // Return the fair rectangle with maximum similarity found.
    return bestRes;
}