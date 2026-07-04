#include "bfs_fair_2d.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// Computes Tversky similarity from point counts.
static double tverskyByCount2D(int count_I,int count_O,int count_inter,double alpha,double beta)
{
    double inter = count_inter;
    double onlyA = count_I - inter;
    double onlyB = count_O - inter;
    double denom = inter + alpha * onlyA + beta * onlyB;
    return denom == 0.0 ? 0.0 : inter / denom;
}

// Static 2D orthogonal range counter.
// Points are sorted by x. Each segment-tree node stores sorted y-values.
class RangeCounter2D
{
private:
    vector<RawPoint> points;
    vector<vector<double>> tree;

    void build(int node,int left,int right)
    {
        if (left == right)
        {
            tree[node].push_back(points[left].y);
            return;
        }

        int mid = left + (right - left) / 2;
        build(node * 2,left,mid);
        build(node * 2 + 1,mid + 1,right);

        const auto &a = tree[node * 2];
        const auto &b = tree[node * 2 + 1];

        tree[node].resize(a.size() + b.size());
        merge(a.begin(),a.end(),b.begin(),b.end(),tree[node].begin());
    }

    int queryNode(int node,int left,int right,int ql,int qr,double y_min,double y_max) const
    {
        if (qr < left || right < ql)
            return 0;

        if (ql <= left && right <= qr)
        {
            const auto &ys = tree[node];
            auto lo = lower_bound(ys.begin(),ys.end(),y_min);
            auto hi = upper_bound(ys.begin(),ys.end(),y_max);
            return static_cast<int>(hi - lo);
        }

        int mid = left + (right - left) / 2;

        return queryNode(node * 2,left,mid,ql,qr,y_min,y_max)
             + queryNode(node * 2 + 1,mid + 1,right,ql,qr,y_min,y_max);
    }

public:
    RangeCounter2D() = default;

    explicit RangeCounter2D(const vector<RawPoint> &input)
    {
        points = input;

        sort(points.begin(),points.end(),[](const RawPoint &a,const RawPoint &b)
        {
            if (a.x != b.x)
                return a.x < b.x;
            return a.y < b.y;
        });

        if (!points.empty())
        {
            tree.resize(points.size() * 4);
            build(1,0,static_cast<int>(points.size()) - 1);
        }
    }

    int count(double x_min,double x_max,double y_min,double y_max) const
    {
        if (points.empty() || x_min > x_max || y_min > y_max)
            return 0;

        auto leftIt = lower_bound(points.begin(),points.end(),x_min,[](const RawPoint &p,double value)
        {
            return p.x < value;
        });

        auto rightIt = upper_bound(points.begin(),points.end(),x_max,[](double value,const RawPoint &p)
        {
            return value < p.x;
        });

        int left = static_cast<int>(leftIt - points.begin());
        int right = static_cast<int>(rightIt - points.begin()) - 1;

        if (left > right)
            return 0;

        return queryNode(1,0,static_cast<int>(points.size()) - 1,left,right,y_min,y_max);
    }
};

struct RectState
{
    double similarity;
    int xl;
    int xr;
    int yl;
    int yr;
    int count_I;
    int count_inter;
    vector<int> colorCounts;
};

struct RectCompare
{
    bool operator()(const RectState &a,const RectState &b) const
    {
        if (a.similarity != b.similarity)
            return a.similarity < b.similarity;

        int spanA = (a.xr - a.xl) + (a.yr - a.yl);
        int spanB = (b.xr - b.xl) + (b.yr - b.yl);

        if (spanA != spanB)
            return spanA > spanB;
        if (a.xl != b.xl)
            return a.xl > b.xl;
        if (a.xr != b.xr)
            return a.xr > b.xr;
        if (a.yl != b.yl)
            return a.yl > b.yl;
        return a.yr > b.yr;
    }
};

struct RectKey
{
    int xl;
    int xr;
    int yl;
    int yr;

    bool operator==(const RectKey &other) const
    {
        return xl == other.xl && xr == other.xr && yl == other.yl && yr == other.yr;
    }
};

struct RectKeyHash
{
    size_t operator()(const RectKey &r) const
    {
        size_t h = 17;
        h = h * 31 + hash<int>{}(r.xl);
        h = h * 31 + hash<int>{}(r.xr);
        h = h * 31 + hash<int>{}(r.yl);
        h = h * 31 + hash<int>{}(r.yr);
        return h;
    }
};

static bool isFairCounts2D(const vector<int> &counts,const unordered_map<string,int> &colorIndex)
{
    auto getCount = [&](const string &color)
    {
        auto it = colorIndex.find(color);
        return it == colorIndex.end() ? 0 : counts[it->second];
    };

    for (const auto &config : diffPairs)
    {
        double diff = config.weightA * getCount(config.colorA)
                    - config.weightB * getCount(config.colorB);

        if (abs(diff) > config.epsilon)
            return false;
    }

    for (const auto &config : ratioPairs)
    {
        int countA = getCount(config.colorA);
        int countB = getCount(config.colorB);

        double cr = config.weightA * countA
                  - config.weightB * countB * (1.0 + config.delta);

        double cb = config.weightB * countB * (1.0 - config.delta)
                  - config.weightA * countA;

        if (cr > 0.0 || cb > 0.0)
            return false;
    }

    return true;
}



optional<Result2D> bfsFair2D(
    RawData2D &raw,
    double qX_min,double qX_max,
    double qY_min,double qY_max,
    double alpha,double beta)
{
    int n = static_cast<int>(raw.pts.size());

    if (n == 0)
        return nullopt;



    vector<double> xs;
    vector<double> ys;

    xs.reserve(n);
    ys.reserve(n);

    for (const auto &p : raw.pts)
    {
        xs.push_back(p.x);
        ys.push_back(p.y);
    }

    sort(xs.begin(),xs.end());
    xs.erase(unique(xs.begin(),xs.end()),xs.end());

    sort(ys.begin(),ys.end());
    ys.erase(unique(ys.begin(),ys.end()),ys.end());

    int nx = static_cast<int>(xs.size());
    int ny = static_cast<int>(ys.size());

    int qXL = static_cast<int>(lower_bound(xs.begin(),xs.end(),qX_min) - xs.begin());
    int qXR = static_cast<int>(upper_bound(xs.begin(),xs.end(),qX_max) - xs.begin()) - 1;
    int qYL = static_cast<int>(lower_bound(ys.begin(),ys.end(),qY_min) - ys.begin());
    int qYR = static_cast<int>(upper_bound(ys.begin(),ys.end(),qY_max) - ys.begin()) - 1;

    if (qXL >= nx || qXR < 0 || qYL >= ny || qYR < 0 ||
        qXL > qXR || qYL > qYR)
    {
        return nullopt;
    }

    unordered_map<string,int> colorIndex;
    vector<vector<RawPoint>> pointsByColor(raw.uniqueColorsList.size());

    for (int i = 0;i < static_cast<int>(raw.uniqueColorsList.size());++i)
        colorIndex[raw.uniqueColorsList[i]] = i;

    for (const auto &p : raw.pts)
    {
        auto it = colorIndex.find(p.color);

        if (it != colorIndex.end())
            pointsByColor[it->second].push_back(p);
    }

    // One counter for all points and one counter per color.
    RangeCounter2D totalCounter(raw.pts);
    vector<RangeCounter2D> colorCounters;
    colorCounters.reserve(pointsByColor.size());

    for (const auto &colorPoints : pointsByColor)
        colorCounters.emplace_back(colorPoints);

    int count_O = totalCounter.count(qX_min,qX_max,qY_min,qY_max);

    if (count_O == 0)
        return nullopt;

    priority_queue<RectState,vector<RectState>,RectCompare> pq;
    unordered_set<RectKey,RectKeyHash> visited;

    auto makeState = [&](int xl,int xr,int yl,int yr) -> RectState
    {
        double x_min = xs[xl];
        double x_max = xs[xr];
        double y_min = ys[yl];
        double y_max = ys[yr];

        int count_I = totalCounter.count(x_min,x_max,y_min,y_max);

        double ix_min = max(x_min,qX_min);
        double ix_max = min(x_max,qX_max);
        double iy_min = max(y_min,qY_min);
        double iy_max = min(y_max,qY_max);

        int count_inter = 0;

        if (ix_min <= ix_max && iy_min <= iy_max)
            count_inter = totalCounter.count(ix_min,ix_max,iy_min,iy_max);

        vector<int> counts(colorCounters.size(),0);

        for (int c = 0;c < static_cast<int>(colorCounters.size());++c)
            counts[c] = colorCounters[c].count(x_min,x_max,y_min,y_max);

        double similarity = tverskyByCount2D(
            count_I,count_O,count_inter,alpha,beta
        );

        return {
            similarity,
            xl,xr,yl,yr,
            count_I,
            count_inter,
            move(counts)
        };
    };

    RectState initial = makeState(qXL,qXR,qYL,qYR);
    
        pq.push(move(initial));
    visited.insert({qXL,qXR,qYL,qYR});

    auto pushNeighbor = [&](int xl,int xr,int yl,int yr)
    {
        if (xl < 0 || xr >= nx || yl < 0 || yr >= ny ||
            xl > xr || yl > yr)
        {
            return;
        }

        RectKey key{xl,xr,yl,yr};

        if (!visited.insert(key).second)
            return;

        RectState state = makeState(xl,xr,yl,yr);

        if (state.count_I == 0)
            return;

        pq.push(move(state));
    };

    // Move in one boundary direction until the enclosed point set changes.
    // Intermediate coordinate boundaries that add/remove no points are equivalent
    // for similarity and fairness, so they are skipped.
    auto pushNextChangingNeighbor = [&](const RectState &cur, int dir)
    {
        int xl = cur.xl;
        int xr = cur.xr;
        int yl = cur.yl;
        int yr = cur.yr;

        while (true)
        {
            if (dir == 0) --xl;
            else if (dir == 1) ++xl;
            else if (dir == 2) ++xr;
            else if (dir == 3) --xr;
            else if (dir == 4) --yl;
            else if (dir == 5) ++yl;
            else if (dir == 6) ++yr;
            else if (dir == 7) --yr;

            if (xl < 0 || xr >= nx || yl < 0 || yr >= ny ||
                xl > xr || yl > yr)
                return;

            int nextCount = totalCounter.count(
                xs[xl], xs[xr], ys[yl], ys[yr]
            );

            if (nextCount != cur.count_I)
            {
                pushNeighbor(xl, xr, yl, yr);
                return;
            }
        }
    };



    // Track the best fair rectangle found during the search.
    // Do not return immediately on the first fair state, because a
    // higher-similarity state may be generated later.

    while (!pq.empty())
    {
        RectState current = pq.top();
        pq.pop();

        bool fair = isFairCounts2D(current.colorCounts,colorIndex);

        if (fair)
        {
            return Result2D{
                xs[current.xl],
                xs[current.xr],
                ys[current.yl],
                ys[current.yr],
                current.similarity
            };
        }
        for (int dir = 0; dir < 8; ++dir)
            pushNextChangingNeighbor(current, dir);
    }
    return nullopt;
}