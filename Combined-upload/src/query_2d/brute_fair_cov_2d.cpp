#include "brute_fair_cov_2d.h"

#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;


static double brute_tversky2D_cov(
    int count_I,
    int count_O,
    int count_inter,
    double alpha,
    double beta)
{
    double inter = count_inter;

    double onlyA =
        count_I - inter;

    double onlyB =
        count_O - inter;

    double denom =
        inter
        + alpha * onlyA
        + beta * onlyB;


    return
        (denom == 0.0)
        ? 0.0
        : inter / denom;
}


// ============================================================
// Check coverage using the color prefix arrays in dataY.
//
// YL and YR are 1-indexed, matching isFair(dataY, YL, YR).
//
// required[c] corresponds to:
// raw.uniqueColorsList[c]
// ============================================================

static bool coverageSatisfied2D(
    const Data &dataY,
    int YL,
    int YR,
    const vector<int> &required)
{
    int numColors =
        static_cast<int>(
            dataY.prefix.size()
        );


    if (required.size() !=
        static_cast<size_t>(numColors)) {

        return false;
    }


    for (int c = 0;
         c < numColors;
         ++c) {

        int count =
            dataY.prefix[c][YR]
            - dataY.prefix[c][YL - 1];


        if (count < required[c]) {
            return false;
        }
    }


    return true;
}


optional<Result2D> brute_force_cov_2d(
    RawData2D &raw,

    double qX_min,
    double qX_max,

    double qY_min,
    double qY_max,

    double alpha,
    double beta,

    const vector<int> &required)
{
    int n =
        static_cast<int>(
            raw.pts.size()
        );


    if (n == 0) {
        return nullopt;
    }


    if (required.size() !=
        raw.uniqueColorsList.size()) {

        cerr
            << "Coverage requirement count does not "
            << "match number of colors.\n";

        return nullopt;
    }


    if (n > 5000) {

        cout
            << "Warning: 2D brute-force coverage on "
            << n
            << " points may take a very long time.\n";
    }


    // ========================================================
    // Count |O|:
    // number of dataset points inside original query rectangle
    // ========================================================

    int count_O = 0;


    for (const RawPoint &p : raw.pts) {

        if (p.x >= qX_min &&
            p.x <= qX_max &&
            p.y >= qY_min &&
            p.y <= qY_max) {

            count_O++;
        }
    }


    if (count_O == 0) {
        return nullopt;
    }


    // ========================================================
    // Sort complete dataset by X
    // ========================================================

    vector<RawPoint> sorted_pts =
        raw.pts;


    sort(
        sorted_pts.begin(),
        sorted_pts.end(),

        [](const RawPoint &a,
           const RawPoint &b) {

            if (a.x != b.x) {
                return a.x < b.x;
            }

            return a.y < b.y;
        }
    );


    double bestT = -1.0;


    Result2D bestRes = {
        0.0,
        0.0,
        0.0,
        0.0,
        -1.0
    };


    // ========================================================
    // Enumerate X intervals
    // ========================================================

    for (int XL = 0;
         XL < n;
         ++XL) {


        // Do not start in the middle of an equal-X group.
        if (XL > 0 &&
            sorted_pts[XL].x ==
            sorted_pts[XL - 1].x) {

            continue;
        }


        for (int XR = XL;
             XR < n;
             ++XR) {


            // Do not end in the middle of an equal-X group.
            if (XR < n - 1 &&
                sorted_pts[XR].x ==
                sorted_pts[XR + 1].x) {

                continue;
            }


            // =================================================
            // Extract this X slice
            // =================================================

            vector<RawPoint> subY;

            subY.reserve(
                XR - XL + 1
            );


            for (int i = XL;
                 i <= XR;
                 ++i) {

                subY.push_back(
                    sorted_pts[i]
                );
            }


            // =================================================
            // Sort X slice by Y
            // =================================================

            sort(
                subY.begin(),
                subY.end(),

                [](const RawPoint &a,
                   const RawPoint &b) {

                    if (a.y != b.y) {
                        return a.y < b.y;
                    }

                    return a.x < b.x;
                }
            );


            int nY =
                static_cast<int>(
                    subY.size()
                );


            if (nY == 0) {
                continue;
            }


            // =================================================
            // Build fairness + color prefix data for this slice
            // =================================================

            Data dataY =
                buildSubsetData(
                    subY,
                    raw
                );


            // =================================================
            // Prefix count for I intersection O
            // =================================================

            vector<int> prefix_interY(
                nY + 1,
                0
            );


            for (int i = 0;
                 i < nY;
                 ++i) {

                prefix_interY[i + 1] =
                    prefix_interY[i];


                const RawPoint &p =
                    subY[i];


                if (p.x >= qX_min &&
                    p.x <= qX_max &&
                    p.y >= qY_min &&
                    p.y <= qY_max) {

                    prefix_interY[i + 1]++;
                }
            }


            // =================================================
            // Enumerate Y intervals
            //
            // YL and YR are 1-indexed because Data/isFair use
            // the same convention as the 1D implementation.
            // =================================================

            for (int YL = 1;
                 YL <= nY;
                 ++YL) {


                // Do not start in the middle of equal-Y values.
                if (YL > 1 &&
                    subY[YL - 1].y ==
                    subY[YL - 2].y) {

                    continue;
                }


                for (int YR = YL;
                     YR <= nY;
                     ++YR) {


                    // Do not end in the middle of equal-Y values.
                    if (YR < nY &&
                        subY[YR - 1].y ==
                        subY[YR].y) {

                        continue;
                    }


                    // ==========================================
                    // Fairness check
                    // ==========================================

                    if (!isFair(
                            dataY,
                            YL,
                            YR)) {

                        continue;
                    }


                    // ==========================================
                    // Coverage check
                    // ==========================================

                    if (!coverageSatisfied2D(
                            dataY,
                            YL,
                            YR,
                            required)) {

                        continue;
                    }


                    // ==========================================
                    // Compute 2D Tversky similarity
                    // ==========================================

                    int count_I =
                        YR - YL + 1;


                    int count_inter =
                        prefix_interY[YR]
                        - prefix_interY[YL - 1];


                    double t =
                        brute_tversky2D_cov(
                            count_I,
                            count_O,
                            count_inter,
                            alpha,
                            beta
                        );


                    if (t > bestT) {

                        bestT = t;


                        bestRes = {

                            sorted_pts[XL].x,

                            sorted_pts[XR].x,

                            subY[YL - 1].y,

                            subY[YR - 1].y,

                            t
                        };
                    }
                }
            }
        }
    }


    if (bestT < 0.0) {
        return nullopt;
    }


    return bestRes;
}