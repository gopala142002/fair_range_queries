#include "kd_tree_opt_cov_2d.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>
using namespace std;

// Computes Tversky similarity between candidate rectangle I and original query O.
static double tversky2D_cov(int count_I,int count_O,int count_inter,double alpha,double beta)
{
    double inter = count_inter;
    double onlyA =count_I - inter;
    double onlyB =count_O - inter;
    double denom =inter+ alpha * onlyA+ beta * onlyB;

    return (denom == 0.0) ? 0.0 : inter / denom;
}

// Finds the smallest right prefix endpoint that satisfies all coverage requirements.
static int findCoverageRight2D(const Data &data,int p,const vector<int> &required)
{
    int n =static_cast<int>(data.keys.size());
    int numColors =static_cast<int>(data.prefix.size());

    if (required.size() != static_cast<size_t>(numColors))
    {
        return -1;
    }

    int low = p + 1;
    int high = n;
    int answer = -1;
    while (low <= high)
    {

        int mid =low + (high - low) / 2;
        bool ok = true;
        for (int c = 0;c < numColors;++c)
        {
            int cnt =data.prefix[c][mid]- data.prefix[c][p];
            if (cnt < required[c])
            {
                ok = false;
                break;
            }
        }
        if (ok)
        {

            answer = mid;
            high = mid - 1;
        }
        else
        {
            low = mid + 1;
        }
    }
    return answer;
}

// Finds the largest left prefix endpoint that still satisfies all coverage requirements.
static int findCoverageLeft2D(const Data &data,int r,const vector<int> &required)
{
    int numColors =static_cast<int>(data.prefix.size());

    if (required.size() != static_cast<size_t>(numColors))
    {
        return -1;
    }
    int low = 0;
    int high = r - 1;
    int answer = -1;
    while (low <= high)
    {
        int mid =low + (high - low) / 2;
        bool ok = true;
        for (int c = 0;c < numColors;++c)
        {
            int cnt =data.prefix[c][r]- data.prefix[c][mid];
            if (cnt < required[c])
            {
                ok = false;
                break;
            }
        }

        if (ok)
        {
            answer = mid;
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }
    return answer;
}

// Searches for the fair and coverage-satisfying rectangle with maximum Tversky similarity.
// X-ranges are enumerated outside; a KD-tree searches feasible Y-ranges inside each X-slab.
optional<Result2D> queryBestKD_opt_cov_2d(RawData2D &raw,double qX_min,double qX_max,double qY_min,double qY_max,double alpha,double beta,const vector<int> &required)
{
    int n_total =static_cast<int>(raw.pts.size());
    if (n_total == 0)
    return nullopt;
    if (required.size()!= raw.uniqueColorsList.size())
    {
        return nullopt;
    }
    // Sort globally by X because the outer search enumerates X-ranges.
    vector<RawPoint> sorted_pts_X =raw.pts;
    sort(sorted_pts_X.begin(),sorted_pts_X.end(),[](const RawPoint &a,const RawPoint &b)
    {
        if (a.x != b.x)
        return a.x < b.x;
        return a.y < b.y;
    });

    // Prefix count of points belonging to the original query rectangle.
    vector<int> prefix_Q(n_total + 1,0);
    for (int i = 0;i < n_total;++i)
    {
        prefix_Q[i + 1] =prefix_Q[i];
        const RawPoint &p =sorted_pts_X[i];

        if (p.x >= qX_min && p.x <= qX_max && p.y >= qY_min && p.y <= qY_max)
        {
            prefix_Q[i + 1]++;
        }
    }

    int count_O =
    prefix_Q[n_total];

    if (count_O == 0)
    return nullopt;

    int qL_X = 0;

    int qR_X =n_total - 1;

    while (qL_X < n_total && sorted_pts_X[qL_X].x < qX_min)
    {
        qL_X++;
    }

    while (qR_X >= 0 && sorted_pts_X[qR_X].x > qX_max)
    {
        qR_X--;
    }

    if (qL_X > qR_X)
    return nullopt;

    double bestT = -1.0;

    Result2D bestRes = {0.0,0.0,0.0,0.0,-1.0};

    // Upper bound for expansion outside the original query X-range.
    auto getUB1 =[&](int X_L,int X_R) -> double
    {
        int A_out =max(0, qL_X - X_L)+max(0, X_R - qR_X);

        if (A_out <= 0)
        return 1.0;

        return static_cast<double>(count_O)/(count_O+ alpha * A_out);
    };

    // Upper bound for contraction that removes original-query points.
    auto getUB2 =[&](int X_L,int X_R) -> double
    {
        if (X_L > X_R)
        return 0.0;
        int Q_rem =prefix_Q[X_R + 1]- prefix_Q[X_L];

        if (Q_rem <= 0)
        return 0.0;

        return static_cast<double>(Q_rem)/(Q_rem+ beta* (count_O - Q_rem));
    };

    // Use the tighter of the expansion and contraction bounds.
    auto getUB =[&](int X_L,int X_R) -> double
    {
        return min(getUB1(X_L, X_R),getUB2(X_L, X_R));
    };

    // For one fixed X-slab, search for the best feasible Y-interval.
    auto evaluateXRange =[&](int X_L,int X_R)
    {
        vector<RawPoint> subY;
        subY.reserve(X_R - X_L + 1);

        for (int i = X_L;i <= X_R;++i)
        {
            subY.push_back(sorted_pts_X[i]);
        }
        sort(subY.begin(),subY.end(),[](const RawPoint &a,const RawPoint &b)
        {
            if (a.y != b.y)
                return a.y < b.y;

            return a.x < b.x;
        });
        int nY =static_cast<int>(subY.size());

        if (nY == 0)
            return;
        int L0_Y = 0;
        int R0_Y = nY - 1;

        while (L0_Y < nY && subY[L0_Y].y < qY_min)
        {
            L0_Y++;
        }

        while (R0_Y >= 0 && subY[R0_Y].y > qY_max)
        {
            R0_Y--;
        }
        if (L0_Y > R0_Y)
            return;

        int qL_Y = L0_Y + 1;

        int qR_Y = R0_Y + 1;

        // Prefix count for candidate intersection with O

        vector<int> prefix_interY(nY + 1,0);

        for (int i = 0;i < nY;++i)
        {
            prefix_interY[i + 1] =prefix_interY[i];
            const RawPoint &p =subY[i];

            if (p.x >= qX_min && p.x <= qX_max && p.y >= qY_min && p.y <= qY_max)
            {
                prefix_interY[i + 1]++;
            }
        }

        int local_count_O = prefix_interY[nY];

        if (local_count_O == 0)
            return;

        // Build fairness prefix Data and KD-tree
        // Important:
        // No fairness-only subset reduction is used here,
        // because coverage may require points outside such a
        // reduced Y subset.

        // Build fairness prefix-space data for the Y-sorted slab.
        Data dataY = buildSubsetData(subY,raw);

        // Build KD-tree over fairness dimensions plus prefix-position dimension.
        KDTree treeY(dataY.points,dataY.m + 1);

        double x_min_range =sorted_pts_X[X_L].x;

        double x_max_range = sorted_pts_X[X_R].x;

        vector<double> lo(dataY.m + 1);

        vector<double> hi(dataY.m + 1);

        // Fix left Y boundary and find right boundary

        // Fix a left prefix endpoint and search for a feasible right endpoint.
        auto tryLeftY =[&](int p)
        {
            // Coverage requires:
            // R >= minimum coverage endpoint.

            int coverageR =findCoverageRight2D(dataY,p,required);

            if (coverageR == -1)
                return;

            const auto &center = dataY.points[p];

            setQueryBounds(dataY,center,lo,hi);

            lo[dataY.m] = coverageR;

            hi[dataY.m] = nY;

            auto [bestR_1, bestR_2] =getBothClosest(treeY,lo,hi,qR_Y,dataY.m);

            for (int bR :{bestR_1, bestR_2})
            {
                if (bR == -1)
                    continue;

                int count_I = bR - p;

                int count_inter = prefix_interY[bR] - prefix_interY[p];

                double t =tversky2D_cov(count_I,count_O,count_inter,alpha,beta);

                if (t > bestT) 
                {
                    bestT = t;
                    bestRes = {x_min_range,x_max_range,subY[p].y,subY[bR - 1].y,t};
                }
            }
        };

        // Fix right Y boundary and find left boundary

        // Fix a right prefix endpoint and search for a feasible left endpoint.
        auto tryRightY =[&](int r)
        {
            // Coverage requires:
            // bL <= largest prefix boundary satisfying coverage.
            int coverageL =findCoverageLeft2D(dataY,r,required);

            if (coverageL == -1)
                return;

            const auto &center = dataY.points[r];

            setQueryBounds(dataY,center,lo,hi,true);

            lo[dataY.m] = 0;

            hi[dataY.m] = coverageL;

            auto [bestL_1, bestL_2] = getBothClosest(treeY,lo,hi,qL_Y - 1,dataY.m);

            for (int bL :{bestL_1, bestL_2})
            {
                if (bL == -1)
                    continue;

                int L =bL + 1;
 
                int count_I = r - L + 1;

                int count_inter =prefix_interY[r] - prefix_interY[L - 1];

                double t =tversky2D_cov(count_I,count_O,count_inter,alpha,beta);

                if (t > bestT) 
                {
                    bestT = t;
                    bestRes = {x_min_range,x_max_range,subY[L - 1].y,subY[r - 1].y,t};
                }
            }
        };

        // Y search phases

        if (qR_Y <= nY - qL_Y) 
        {
            // LEFT SIDE smaller
            // Expand left
            for (int p = qL_Y - 2;p >= 0;--p)
            {
                int L =p + 1;

                double ub =static_cast<double>(local_count_O)/(local_count_O+alpha* (qL_Y - L));
                if (ub <= bestT)
                    break;
                tryLeftY(p);
            }

            // Contract from left

            for (int p = qL_Y - 1;p <= qR_Y - 1;++p)
            {
                int L =p + 1;

                int Q_rem = prefix_interY[nY]-prefix_interY[L - 1];

                if (Q_rem <= 0)
                    break;

                double ub =static_cast<double>(Q_rem)/(Q_rem+beta* (local_count_O- Q_rem));
                if (ub <= bestT)
                    break;
                tryLeftY(p);
            }

        } 
        else 
        {

            // RIGHT SIDE smaller

            // Expand right

            for (int r = qR_Y;r <= nY;++r)
            {
                double ub =static_cast<double>(local_count_O)/(local_count_O+alpha* (r - qR_Y));
                if (ub <= bestT)
                    break;
                tryRightY(r);
            }

            // Contract from right
            for (int r = qR_Y - 1;r >= qL_Y;--r)
            {
                int Q_rem =prefix_interY[r];
                if (Q_rem <= 0)
                    break;
                double ub =static_cast<double>(Q_rem)/(Q_rem+beta* (local_count_O- Q_rem));
                if (ub <= bestT)
                    break;
                tryRightY(r);
            }
        }
    };

    // Outer X search

    // Phase 1:
    // X_L expands left

    for (int X_L = qL_X;X_L >= 0;--X_L)
    {
        if (X_L < qL_X && getUB1(X_L,qR_X) <= bestT)
        {
            break;
        }

        // Expand X_R right

        for (int X_R = qR_X;X_R < n_total;++X_R)
        {
            if (getUB(X_L,X_R) <= bestT)
            {
                break;
            }
            evaluateXRange(X_L,X_R);
        }

        // Contract X_R left

        for (int X_R = qR_X - 1;
        X_R >= X_L;
        --X_R)
        {
            if (getUB(
            X_L,
            X_R
            ) <= bestT)
            {
                break;
            }

            evaluateXRange(
            X_L,
            X_R
            );
        }
    }

    // Phase 2:
    // X_L contracts inward

    for (int X_L = qL_X + 1;X_L <= qR_X;++X_L)
    {
        if (getUB2(X_L,n_total - 1) <= bestT)
        {
            break;
        }

        // Expand X_R right
        for (int X_R =max(X_L, qR_X);X_R < n_total;++X_R)
        {
            if (getUB(X_L,X_R) <= bestT)
            {
                break;
            }
            evaluateXRange(X_L,X_R);
        }

        // Contract X_R left

        for (int X_R =min(n_total - 1,qR_X - 1);X_R >= X_L;--X_R)
        {
            if (getUB(X_L,X_R) <= bestT)
            {
                break;
            }
            evaluateXRange(X_L,X_R);
        }
    }

    if(bestT < 0.0)
        return nullopt;

    return bestRes;
}