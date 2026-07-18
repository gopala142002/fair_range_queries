#include "brute_2d.h"

#include <algorithm>
#include <iostream>

using namespace std;

static double brute_tversky2D(
    int count_I,
    int count_O,
    int count_inter,
    double alpha,
    double beta)
{
    double inter = count_inter;
    double onlyA = count_I - inter;
    double onlyB = count_O - inter;

    double denom =
        inter
        + alpha * onlyA
        + beta * onlyB;

    return denom == 0.0
        ? 0.0
        : inter / denom;
}


optional<Result2D> brute_force_2d(
    RawData2D &raw,
    double qX_min,
    double qX_max,
    double qY_min,
    double qY_max,
    double alpha,
    double beta)
{
    int n = static_cast<int>(raw.pts.size());

    if (n == 0)
        return nullopt;

    if (n > 5000)
    {
        cout
            << "Warning: Brute force on "
            << n
            << " points might take a long time.\n";
    }

    int count_O = 0;

    for (const auto &p : raw.pts)
    {
        if (p.x >= qX_min &&
            p.x <= qX_max &&
            p.y >= qY_min &&
            p.y <= qY_max)
        {
            ++count_O;
        }
    }

    if (count_O == 0)
        return nullopt;

    vector<RawPoint> sorted_pts = raw.pts;

    sort(
        sorted_pts.begin(),
        sorted_pts.end(),
        [](const RawPoint &a, const RawPoint &b)
        {
            if (a.x != b.x)
                return a.x < b.x;

            return a.y < b.y;
        }
    );

    double bestT = -1.0;

    Result2D bestRes =
        {0.0, 0.0, 0.0, 0.0, -1.0};

    for (int XL = 0; XL < n; ++XL)
    {
        // Do not place the left X boundary inside a duplicate-X group.
        if (XL > 0 &&
            sorted_pts[XL].x == sorted_pts[XL - 1].x)
        {
            continue;
        }

        for (int XR = XL; XR < n; ++XR)
        {
            // Do not place the right X boundary inside a duplicate-X group.
            if (XR < n - 1 &&
                sorted_pts[XR].x == sorted_pts[XR + 1].x)
            {
                continue;
            }

            vector<RawPoint> subY;
            subY.reserve(XR - XL + 1);

            for (int i = XL; i <= XR; ++i)
            {
                subY.push_back(sorted_pts[i]);
            }

            sort(
                subY.begin(),
                subY.end(),
                [](const RawPoint &a, const RawPoint &b)
                {
                    if (a.y != b.y)
                        return a.y < b.y;

                    return a.x < b.x;
                }
            );

            int nY = static_cast<int>(subY.size());

            if (nY == 0)
                continue;

            Data dataY =
                buildSubsetData(subY, raw);

            vector<int> prefix_interY(
                nY + 1,
                0
            );

            for (int i = 0; i < nY; ++i)
            {
                prefix_interY[i + 1] =
                    prefix_interY[i];

                if (subY[i].x >= qX_min &&
                    subY[i].x <= qX_max &&
                    subY[i].y >= qY_min &&
                    subY[i].y <= qY_max)
                {
                    ++prefix_interY[i + 1];
                }
            }

            for (int YL = 1; YL <= nY; ++YL)
            {
                // Do not place the lower Y boundary inside
                // a duplicate-Y group.
                if (YL > 1 &&
                    subY[YL - 1].y ==
                    subY[YL - 2].y)
                {
                    continue;
                }

                for (int YR = YL; YR <= nY; ++YR)
                {
                    // Do not place the upper Y boundary inside
                    // a duplicate-Y group.
                    if (YR < nY &&
                        subY[YR - 1].y ==
                        subY[YR].y)
                    {
                        continue;
                    }

                    if (!isFair(dataY, YL, YR))
                        continue;

                    int count_I =
                        YR - YL + 1;

                    int count_inter =
                        prefix_interY[YR]
                        - prefix_interY[YL - 1];

                    double t =
                        brute_tversky2D(
                            count_I,
                            count_O,
                            count_inter,
                            alpha,
                            beta
                        );

                    if (t > bestT)
                    {
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

    if (bestT < 0.0)
        return nullopt;

    return bestRes;
}