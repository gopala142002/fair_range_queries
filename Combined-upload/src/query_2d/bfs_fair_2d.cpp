#include "bfs_fair_2d.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;


// ============================================================
// 2D Tversky similarity
// Same definition used by brute_2d.cpp and kd_tree_opt_2d.cpp
// ============================================================

static double tverskyByCount2D(
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
        inter +
        alpha * onlyA +
        beta * onlyB;

    return (denom == 0.0)
        ? 0.0
        : inter / denom;
}


// ============================================================
// BFS state
//
// Rectangle:
//
// X = [xs[xl], xs[xr]]
// Y = [ys[yl], ys[yr]]
// ============================================================

struct RectState {
    double similarity;

    int xl;
    int xr;

    int yl;
    int yr;
};


// ============================================================
// Priority queue comparator
//
// Same philosophy as 1D BFS:
//
// 1. Higher similarity first
// 2. Smaller boundary-index span first
// 3. Lexicographic tie-breaking
// ============================================================

struct RectCompare {

    bool operator()(
        const RectState &a,
        const RectState &b) const
    {
        if (a.similarity != b.similarity)
            return a.similarity < b.similarity;

        int spanA =
            (a.xr - a.xl) +
            (a.yr - a.yl);

        int spanB =
            (b.xr - b.xl) +
            (b.yr - b.yl);

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


// ============================================================
// Rectangle key for visited set
// ============================================================

struct RectKey {

    int xl;
    int xr;
    int yl;
    int yr;

    bool operator==(const RectKey &other) const {
        return
            xl == other.xl &&
            xr == other.xr &&
            yl == other.yl &&
            yr == other.yr;
    }
};


struct RectKeyHash {

    size_t operator()(const RectKey &r) const {

        size_t h = 17;

        h = h * 31 + hash<int>{}(r.xl);
        h = h * 31 + hash<int>{}(r.xr);
        h = h * 31 + hash<int>{}(r.yl);
        h = h * 31 + hash<int>{}(r.yr);

        return h;
    }
};


// ============================================================
// Check whether point lies inside rectangle
// ============================================================

static inline bool insideRectangle(
    const RawPoint &p,
    double x_min,
    double x_max,
    double y_min,
    double y_max)
{
    return
        p.x >= x_min &&
        p.x <= x_max &&
        p.y >= y_min &&
        p.y <= y_max;
}


// ============================================================
// Evaluate rectangle
//
// Computes:
//
// count_I       = points in candidate rectangle
// count_inter   = points in candidate AND original query
// color counts  = used for fairness check
// ============================================================

static void evaluateRectangle(
    const RawData2D &raw,

    double x_min,
    double x_max,
    double y_min,
    double y_max,

    double qX_min,
    double qX_max,
    double qY_min,
    double qY_max,

    int &count_I,
    int &count_inter,

    unordered_map<string, int> &colorCounts)
{
    count_I = 0;
    count_inter = 0;

    colorCounts.clear();

    for (const auto &p : raw.pts) {

        if (!insideRectangle(
                p,
                x_min,
                x_max,
                y_min,
                y_max))
        {
            continue;
        }

        count_I++;

        colorCounts[p.color]++;

        if (insideRectangle(
                p,
                qX_min,
                qX_max,
                qY_min,
                qY_max))
        {
            count_inter++;
        }
    }
}


// ============================================================
// Fairness check from color counts
//
// Same equations used by common.cpp / kd_tree_opt_2d.cpp
// ============================================================

static bool isFairCounts2D(
    const unordered_map<string, int> &counts)
{
    // --------------------------------------------------------
    // Difference constraints
    // --------------------------------------------------------

    for (const auto &config : diffPairs) {

        int countA = 0;
        int countB = 0;

        auto itA = counts.find(config.colorA);

        if (itA != counts.end())
            countA = itA->second;


        auto itB = counts.find(config.colorB);

        if (itB != counts.end())
            countB = itB->second;


        double diff =
            config.weightA * countA -
            config.weightB * countB;


        if (abs(diff) > config.epsilon)
            return false;
    }


    // --------------------------------------------------------
    // Ratio constraints
    // --------------------------------------------------------

    for (const auto &config : ratioPairs) {

        int countA = 0;
        int countB = 0;


        auto itA = counts.find(config.colorA);

        if (itA != counts.end())
            countA = itA->second;


        auto itB = counts.find(config.colorB);

        if (itB != counts.end())
            countB = itB->second;


        double cr =
            config.weightA * countA
            -
            config.weightB *
            countB *
            (1.0 + config.delta);


        double cb =
            config.weightB *
            countB *
            (1.0 - config.delta)
            -
            config.weightA *
            countA;


        if (cr > 0.0)
            return false;

        if (cb > 0.0)
            return false;
    }


    return true;
}


// ============================================================
// Main BFS Fair 2D
// ============================================================

optional<Result2D> bfsFair2D(
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


    // ========================================================
    // STEP 1
    //
    // Collect all distinct X and Y coordinates.
    //
    // Candidate boundaries always come from actual data points.
    // ========================================================

    vector<double> xs;
    vector<double> ys;

    xs.reserve(n);
    ys.reserve(n);


    for (const auto &p : raw.pts) {

        xs.push_back(p.x);
        ys.push_back(p.y);
    }


    sort(xs.begin(), xs.end());

    xs.erase(
        unique(xs.begin(), xs.end()),
        xs.end()
    );


    sort(ys.begin(), ys.end());

    ys.erase(
        unique(ys.begin(), ys.end()),
        ys.end()
    );


    int nx = static_cast<int>(xs.size());
    int ny = static_cast<int>(ys.size());


    // ========================================================
    // STEP 2
    //
    // Find initial rectangle corresponding to the input query.
    // ========================================================

    int qXL =
        static_cast<int>(
            lower_bound(
                xs.begin(),
                xs.end(),
                qX_min
            ) - xs.begin()
        );


    int qXR =
        static_cast<int>(
            upper_bound(
                xs.begin(),
                xs.end(),
                qX_max
            ) - xs.begin()
        ) - 1;


    int qYL =
        static_cast<int>(
            lower_bound(
                ys.begin(),
                ys.end(),
                qY_min
            ) - ys.begin()
        );


    int qYR =
        static_cast<int>(
            upper_bound(
                ys.begin(),
                ys.end(),
                qY_max
            ) - ys.begin()
        ) - 1;


    if (qXL >= nx ||
        qXR < 0 ||
        qYL >= ny ||
        qYR < 0 ||
        qXL > qXR ||
        qYL > qYR)
    {
        return nullopt;
    }


    // ========================================================
    // STEP 3
    //
    // Count |O|:
    // number of points in original query rectangle.
    // ========================================================

    int count_O = 0;


    for (const auto &p : raw.pts) {

        if (insideRectangle(
                p,
                qX_min,
                qX_max,
                qY_min,
                qY_max))
        {
            count_O++;
        }
    }


    if (count_O == 0)
        return nullopt;


    // ========================================================
    // STEP 4
    //
    // Priority queue and visited states.
    // ========================================================

    priority_queue<
        RectState,
        vector<RectState>,
        RectCompare
    > pq;


    unordered_set<
        RectKey,
        RectKeyHash
    > visited;


    // Exact query rectangle has similarity 1.
    pq.push({
        1.0,
        qXL,
        qXR,
        qYL,
        qYR
    });


    visited.insert({
        qXL,
        qXR,
        qYL,
        qYR
    });


    // ========================================================
    // Helper to generate and insert a neighbor.
    // ========================================================

    auto pushNeighbor =
        [&](int xl,
            int xr,
            int yl,
            int yr)
    {
        // Invalid rectangle.
        if (xl < 0 ||
            xr >= nx ||
            yl < 0 ||
            yr >= ny ||
            xl > xr ||
            yl > yr)
        {
            return;
        }


        RectKey key{
            xl,
            xr,
            yl,
            yr
        };


        if (visited.find(key) != visited.end())
            return;


        visited.insert(key);


        int count_I = 0;
        int count_inter = 0;

        unordered_map<string, int> colorCounts;


        evaluateRectangle(
            raw,

            xs[xl],
            xs[xr],

            ys[yl],
            ys[yr],

            qX_min,
            qX_max,

            qY_min,
            qY_max,

            count_I,
            count_inter,

            colorCounts
        );


        // Empty candidate rectangle is not useful.
        if (count_I == 0)
            return;


        double similarity =
            tverskyByCount2D(
                count_I,
                count_O,
                count_inter,
                alpha,
                beta
            );


        pq.push({
            similarity,
            xl,
            xr,
            yl,
            yr
        });
    };


    // ========================================================
    // STEP 5
    //
    // Best-first search.
    //
    // Equivalent idea to 1D BFS:
    //
    // 1D:
    //      [L,R]
    //
    //      L expand
    //      L contract
    //      R expand
    //      R contract
    //
    //
    // 2D:
    //      [XL,XR] x [YL,YR]
    //
    //      XL expand / contract
    //      XR expand / contract
    //      YL expand / contract
    //      YR expand / contract
    // ========================================================

    while (!pq.empty()) {

        RectState current = pq.top();
        pq.pop();


        double x_min = xs[current.xl];
        double x_max = xs[current.xr];

        double y_min = ys[current.yl];
        double y_max = ys[current.yr];


        // ----------------------------------------------------
        // Compute color counts for fairness.
        // ----------------------------------------------------

        int count_I = 0;
        int count_inter = 0;

        unordered_map<string, int> colorCounts;


        evaluateRectangle(
            raw,

            x_min,
            x_max,

            y_min,
            y_max,

            qX_min,
            qX_max,

            qY_min,
            qY_max,

            count_I,
            count_inter,

            colorCounts
        );


        // ----------------------------------------------------
        // Same behavior as 1D BFS:
        //
        // Return the first fair candidate popped from the
        // similarity-priority queue.
        // ----------------------------------------------------

        if (isFairCounts2D(colorCounts)) {

            return Result2D{
                x_min,
                x_max,
                y_min,
                y_max,
                current.similarity
            };
        }


        // ====================================================
        // Generate 8 neighbors.
        // ====================================================


        // ----------------------------------------------------
        // X-left boundary
        // ----------------------------------------------------

        // Expand left
        pushNeighbor(
            current.xl - 1,
            current.xr,
            current.yl,
            current.yr
        );


        // Contract left
        pushNeighbor(
            current.xl + 1,
            current.xr,
            current.yl,
            current.yr
        );


        // ----------------------------------------------------
        // X-right boundary
        // ----------------------------------------------------

        // Expand right
        pushNeighbor(
            current.xl,
            current.xr + 1,
            current.yl,
            current.yr
        );


        // Contract right
        pushNeighbor(
            current.xl,
            current.xr - 1,
            current.yl,
            current.yr
        );


        // ----------------------------------------------------
        // Y-lower boundary
        // ----------------------------------------------------

        // Expand downward
        pushNeighbor(
            current.xl,
            current.xr,
            current.yl - 1,
            current.yr
        );


        // Contract upward
        pushNeighbor(
            current.xl,
            current.xr,
            current.yl + 1,
            current.yr
        );


        // ----------------------------------------------------
        // Y-upper boundary
        // ----------------------------------------------------

        // Expand upward
        pushNeighbor(
            current.xl,
            current.xr,
            current.yl,
            current.yr + 1
        );


        // Contract downward
        pushNeighbor(
            current.xl,
            current.xr,
            current.yl,
            current.yr - 1
        );
    }


    return nullopt;
}