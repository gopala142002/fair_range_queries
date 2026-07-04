#include "brute_2d.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

static double brute_tversky2D(int count_I, int count_O, int count_inter, double alpha, double beta) 
{
    double inter = count_inter;
    double onlyA = count_I - inter;
    double onlyB = count_O - inter;
    double denom = inter + alpha * onlyA + beta * onlyB;
    return denom == 0.0 ? 0.0 : inter / denom;
}

optional<Result2D> brute_force_2d(RawData2D &raw,double qX_min, double qX_max,double qY_min, double qY_max,double alpha, double beta) 
{

    // auto search_start_time = std::chrono::high_resolution_clock::now();
    int n = raw.pts.size();
    if (n == 0) 
        return nullopt;

    if (n > 5000) 
    {
        std::cout << "Warning: Brute force on " << n << " points might take a long time.\n";
    }

    int count_O = 0;
    for (int i = 0; i < n; ++i) 
    {
        if (raw.pts[i].x >= qX_min && raw.pts[i].x <= qX_max &&raw.pts[i].y >= qY_min && raw.pts[i].y <= qY_max) 
        {
            count_O++;
        }
    }

    if (count_O == 0) return std::nullopt; 

    vector<RawPoint> sorted_pts = raw.pts;
    sort(sorted_pts.begin(), sorted_pts.end(), [](const RawPoint& a, const RawPoint& b) 
    {
        return a.x < b.x;
    });

    double bestT = -1.0;
    Result2D bestRes = {0, 0, 0, 0, -1.0};

    for (int XL = 0; XL < n; ++XL) 
    {
        for (int XR = XL; XR < n; ++XR) 
        {

            if (XR < n - 1 && sorted_pts[XR].x == sorted_pts[XR+1].x) 
                continue;
            if (XL > 0 && sorted_pts[XL].x == sorted_pts[XL-1].x) 
                continue;

            vector<RawPoint> subY;
            for (int i = XL; i <= XR; ++i) 
            {
                subY.push_back(sorted_pts[i]);
            }
            sort(subY.begin(), subY.end(), [](const RawPoint& a, const RawPoint& b) 
            {
                return a.y < b.y;
            });

            int nY = subY.size();
            if (nY == 0) 
                continue;


            Data dataY = buildSubsetData(subY, raw);
            vector<int> prefix_interY(nY + 1, 0);
            for (int i = 0; i < nY; ++i) 
            {
                prefix_interY[i+1] = prefix_interY[i];
                if (subY[i].x >= qX_min && subY[i].x <= qX_max && subY[i].y >= qY_min && subY[i].y <= qY_max) 
                {
                    prefix_interY[i+1]++;
                }
            }

            for (int YL = 1; YL <= nY; ++YL)
            {
                for (int YR = YL; YR <= nY; ++YR) 
                {
                    
                    bool fair = isFair(dataY, YL, YR);

                    if (!fair) 
                        continue;

                    int count_I = YR - YL + 1;
                    int count_inter = prefix_interY[YR] - prefix_interY[YL - 1];

                    double t = brute_tversky2D(count_I, count_O, count_inter, alpha, beta);

                    if (t > bestT) 
                    {
                        bestT = t;
                        bestRes = {sorted_pts[XL].x, sorted_pts[XR].x, subY[YL-1].y, subY[YR-1].y, t};
                    }
                }
            }
        }
    }

    if (bestT < 0) 
        return nullopt;
    return bestRes;
}
