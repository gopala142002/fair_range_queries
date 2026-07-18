#include "brute_fair_cov_2d.h"

#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;


// Computes Tversky similarity between:
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
static double brute_tversky2D_cov(int count_I,int count_O,int count_inter,double alpha,double beta)
{
    double inter = count_inter;

    // Points inside candidate rectangle I
    // but outside original query rectangle O.
    double onlyA =count_I - inter;

    // Points inside original query rectangle O
    // but outside candidate rectangle I.
    double onlyB =count_O - inter;

    // Denominator of Tversky similarity.
    double denom =inter+ alpha * onlyA+ beta * onlyB;

    // Avoid division by zero.
    return(denom == 0.0)? 0.0: inter / denom;
}


// Checks whether the candidate y-interval satisfies
// the minimum coverage requirement for every color.
//
// dataY.prefix[c][k] stores the prefix count of color c
// among the first k points in the y-sorted subset.
//
// Therefore:
//
// prefix[c][YR] - prefix[c][YL - 1]
//
// gives the number of points of color c in [YL, YR].
static bool coverageSatisfied2D(const Data &dataY,int YL,int YR,const vector<int> &required)
{
    // Number of colors represented in the prefix structure.
    int numColors =static_cast<int>(dataY.prefix.size());


    // There must be exactly one coverage requirement
    // for every color.
    if (required.size() !=static_cast<size_t>(numColors)) 
    {
        return false;
    }


    // Check the minimum required count for every color.
    for (int c = 0;c < numColors;++c) 
    {
        // Number of points of color c inside [YL, YR].
        int count =dataY.prefix[c][YR]- dataY.prefix[c][YL - 1];


        // Coverage requirement for this color is not satisfied.
        if (count < required[c]) 
        {
            return false;
        }
    }


    // Coverage requirements are satisfied for all colors.
    return true;
}


// Brute-force search for the 2D rectangle having maximum
// Tversky similarity with the original query rectangle,
// subject to both:
//
// 1. Fairness constraints.
// 2. Coverage constraints.
//
// Main procedure:
//
// 1. Count points in the original query rectangle.
// 2. Sort all dataset points by x-coordinate.
// 3. Enumerate every valid x-range [XL, XR].
// 4. Collect points inside the current x-slab.
// 5. Sort those points by y-coordinate.
// 6. Build prefix structures for fairness and coverage.
// 7. Enumerate every valid y-range [YL, YR].
// 8. Check fairness.
// 9. Check coverage.
// 10. Compute Tversky similarity.
// 11. Keep the feasible rectangle with maximum similarity.
optional<Result2D> brute_force_cov_2d(RawData2D &raw,double qX_min,double qX_max,double qY_min,double qY_max,double alpha,double beta,const vector<int> &required)
{
    // Total number of points in the dataset.
    int n =static_cast<int>(raw.pts.size());


    // Empty dataset means no rectangle can be returned.
    if (n == 0) 
    {
        return nullopt;
    }


    // The required coverage vector must contain one
    // minimum count for every unique color in the dataset.
    if (required.size() !=raw.uniqueColorsList.size()) 
    {
        cerr<< "Coverage requirement count does not "<< "match number of colors.\n";
        return nullopt;
    }


    // Warn the user because exhaustive 2D rectangle enumeration
    // can be extremely expensive for large datasets.
    if (n > 5000) 
    {
        cout<< "Warning: 2D brute-force coverage on "<< n << " points may take a very long time.\n";
    }


    // count_O = number of points inside the original query rectangle O.
    int count_O = 0;


    // Count all points belonging to the original query rectangle.
    for (const RawPoint &p : raw.pts) 
    {
        if (p.x >= qX_min &&p.x <= qX_max &&p.y >= qY_min &&p.y <= qY_max) 
        {
            count_O++;
        }
    }


    // If the original query rectangle contains no points,
    // similarity-based search is not performed.
    if (count_O == 0) 
    {
        return nullopt;
    }


    // Create a copy of all dataset points.
    vector<RawPoint> sorted_pts =
        raw.pts;


    // Sort all points primarily by x-coordinate.
    //
    // If two points have the same x-coordinate,
    // sort them by y-coordinate for deterministic ordering.
    sort(sorted_pts.begin(),sorted_pts.end(),[](const RawPoint &a,const RawPoint &b) 
    {
            if (a.x != b.x) 
            {
                return a.x < b.x;
            }

            // Tie-break equal x-coordinates using y-coordinate.
            return a.y < b.y;
        }
    );


    // Best Tversky similarity found so far.
    double bestT = -1.0;


    // Stores the best feasible rectangle found so far.
    Result2D bestRes ={0.0,0.0,0.0,0.0,-1.0};


    // Enumerate every possible left x-boundary.
    for (int XL = 0;XL < n;++XL) 
    {
        // If this point has the same x-coordinate as the previous point,
        // skip it as a left boundary.
        //
        // The left boundary should begin at the first point
        // having this x-coordinate.
        if (XL > 0 &&sorted_pts[XL].x ==sorted_pts[XL - 1].x) 
        {
            continue;
        }


        // Enumerate every possible right x-boundary.
        for (int XR = XL;XR < n;++XR) 
        {
            // If the next point has the same x-coordinate,
            // skip the current XR.
            //
            // The right boundary should end at the last point
            // having this x-coordinate.
            if (XR < n - 1 &&sorted_pts[XR].x ==sorted_pts[XR + 1].x) 
            {
                continue;
            }


            // Store all points belonging to the current x-slab.
            vector<RawPoint> subY;


            // Reserve memory for exactly the maximum number
            // of points that will be inserted.
            subY.reserve(XR - XL + 1);


            // Copy all points from the current x-range.
            for (int i = XL;i <= XR;++i) 
            {
                subY.push_back(sorted_pts[i]);
            }


            // Sort points of the current x-slab primarily
            // by y-coordinate.
            //
            // Equal y-coordinates are ordered by x-coordinate.
            sort(subY.begin(),subY.end(),[](const RawPoint &a,const RawPoint &b) 
            {
                if (a.y != b.y) 
                {
                    return a.y < b.y;
                }

                    // Tie-break equal y-coordinates using x-coordinate.
                    return a.x < b.x;
                }
            );


            // Number of points in the current x-slab.
            int nY =static_cast<int>(subY.size());


            // Safety check for an empty x-slab.
            if (nY == 0) 
            {
                continue;
            }


            // Build prefix data structures required by isFair()
            // and coverage checking.
            //
            // The prefix structure stores cumulative counts
            // for every color in the y-sorted subset.
            Data dataY =buildSubsetData(subY,raw);


            // Prefix array for intersection counts.
            //
            // prefix_interY[k] =
            // number of points belonging to the original query rectangle
            // among subY[0 ... k-1].
            vector<int> prefix_interY(nY + 1,0);


            // Build the intersection prefix array.
            for (int i = 0;i < nY;++i) 
            {
                // Copy previous prefix value.
                prefix_interY[i + 1] =prefix_interY[i];


                // Reference to the current point.
                const RawPoint &p = subY[i];


                // If the point lies inside the original query rectangle,
                // increment the intersection prefix count.
                if (p.x >= qX_min && p.x <= qX_max && p.y >= qY_min &&p.y <= qY_max) 
                {
                    prefix_interY[i + 1]++;
                }
            }

            // Enumerate every possible lower y-boundary.
            //
            // YL uses 1-based indexing because the prefix
            // data structures use that convention.
            for (int YL = 1;YL <= nY;++YL) {

                // If the current point has the same y-coordinate
                // as the previous point, skip this YL.
                //
                // A valid lower geometric boundary should begin
                // at the first point having this y-coordinate.
                if (YL > 1 &&subY[YL - 1].y ==subY[YL - 2].y) 
                {
                    continue;
                }
                // Enumerate every possible upper y-boundary.
                for (int YR = YL;YR <= nY;++YR) 
                {
                    // If the next point has the same y-coordinate,
                    // skip the current YR.
                    //
                    // A valid upper geometric boundary should end
                    // at the last point having this y-coordinate.
                    if (YR < nY &&subY[YR - 1].y ==subY[YR].y) 
                    {
                        continue;
                    }

                    // Check whether the candidate rectangle
                    // satisfies all fairness constraints.
                    if (!isFair(dataY,YL,YR)) 
                    {
                        continue;
                    }

                    // Check whether the candidate rectangle satisfies
                    // the minimum required count for every color.
                    if (!coverageSatisfied2D(dataY,YL,YR,required)) 
                    {
                        continue;
                    }
                    // Number of points in the candidate rectangle.
                    //
                    // Since [YL, YR] is inclusive:
                    //
                    // count = YR - YL + 1
                    int count_I =YR - YL + 1;


                    // Compute the number of candidate points that are
                    // also inside the original query rectangle.
                    //
                    // This is obtained in O(1) using the prefix array.
                    int count_inter =prefix_interY[YR]- prefix_interY[YL - 1];


                    // Compute Tversky similarity between the candidate
                    // rectangle and the original query rectangle.
                    double t =brute_tversky2D_cov(count_I,count_O,count_inter,alpha,beta);


                    // Update the best result if the current feasible
                    // rectangle has higher similarity.
                    if (t > bestT) 
                    {
                        // Store new best similarity.
                        bestT = t;


                        // Store actual rectangle boundaries:
                        //
                        // x_min = sorted_pts[XL].x
                        // x_max = sorted_pts[XR].x
                        // y_min = subY[YL - 1].y
                        // y_max = subY[YR - 1].y
                        //
                        // YL and YR are 1-based, therefore -1 is used
                        // when accessing the subY vector.
                        bestRes = {sorted_pts[XL].x,sorted_pts[XR].x,subY[YL - 1].y,subY[YR - 1].y,t};
                    }
                }
            }
        }
    }
    // If bestT is still negative, no rectangle satisfying
    // both fairness and coverage was found.
    if (bestT < 0.0) 
    {
        return nullopt;
    }
    // Return the feasible rectangle with maximum Tversky similarity.
    return bestRes;
}