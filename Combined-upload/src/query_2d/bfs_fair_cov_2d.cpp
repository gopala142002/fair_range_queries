#include "bfs_fair_cov_2d.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;


// ============================================================
// 2D Tversky similarity
// ============================================================

static double tverskyByCount2D_Cov(
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

    return (denom == 0.0)
        ? 0.0
        : inter / denom;
}


// ============================================================
// BFS rectangle state
// ============================================================

struct RectStateCov {

    double similarity;

    int xl;
    int xr;

    int yl;
    int yr;
};


// ============================================================
// Priority queue comparator
// ============================================================

struct RectCompareCov {

    bool operator()(
        const RectStateCov &a,
        const RectStateCov &b) const
    {
        if (a.similarity != b.similarity)
            return a.similarity < b.similarity;

        int spanA =
            (a.xr - a.xl)
            + (a.yr - a.yl);

        int spanB =
            (b.xr - b.xl)
            + (b.yr - b.yl);

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

struct RectKeyCov {

    int xl;
    int xr;

    int yl;
    int yr;

    bool operator==(
        const RectKeyCov &other) const
    {
        return
            xl == other.xl
            && xr == other.xr
            && yl == other.yl
            && yr == other.yr;
    }
};


struct RectKeyHashCov {

    size_t operator()(
        const RectKeyCov &r) const
    {
        size_t h = 17;

        h = h * 31 + hash<int>{}(r.xl);
        h = h * 31 + hash<int>{}(r.xr);
        h = h * 31 + hash<int>{}(r.yl);
        h = h * 31 + hash<int>{}(r.yr);

        return h;
    }
};


// ============================================================
// Check whether a point is inside a rectangle
// ============================================================

static inline bool insideRectangleCov(
    const RawPoint &p,
    double x_min,
    double x_max,
    double y_min,
    double y_max)
{
    return
        p.x >= x_min
        && p.x <= x_max
        && p.y >= y_min
        && p.y <= y_max;
}


// ============================================================
// Evaluate rectangle
//
// Computes:
//
// count_I
// count_inter
// per-color counts
// ============================================================

static void evaluateRectangleCov(
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

        if (!insideRectangleCov(
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


        if (insideRectangleCov(
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
// ============================================================

static bool isFairCounts2D_Cov(
    const unordered_map<string, int> &counts)
{
    // Difference constraints

    for (const auto &config : diffPairs) {

        int countA = 0;
        int countB = 0;


        auto itA =
            counts.find(config.colorA);

        if (itA != counts.end())
            countA = itA->second;


        auto itB =
            counts.find(config.colorB);

        if (itB != counts.end())
            countB = itB->second;


        double diff =
            config.weightA * countA
            -
            config.weightB * countB;


        if (abs(diff) > config.epsilon)
            return false;
    }


    // Ratio constraints

    for (const auto &config : ratioPairs) {

        int countA = 0;
        int countB = 0;


        auto itA =
            counts.find(config.colorA);

        if (itA != counts.end())
            countA = itA->second;


        auto itB =
            counts.find(config.colorB);

        if (itB != counts.end())
            countB = itB->second;


        double cr =
            config.weightA * countA
            -
            config.weightB
            * countB
            * (1.0 + config.delta);


        double cb =
            config.weightB
            * countB
            * (1.0 - config.delta)
            -
            config.weightA * countA;


        if (cr > 0.0)
            return false;

        if (cb > 0.0)
            return false;
    }


    return true;
}


// ============================================================
// Coverage check
//
// required[c] corresponds to:
//
// raw.uniqueColorsList[c]
// ============================================================

static bool coverageSatisfied2D_BFS(
    const unordered_map<string, int> &counts,

    const vector<string> &uniqueColors,

    const vector<int> &required)
{
    if (required.size() != uniqueColors.size())
        return false;


    for (size_t c = 0;
         c < uniqueColors.size();
         ++c)
    {
        int count = 0;


        auto it =
            counts.find(
                uniqueColors[c]
            );


        if (it != counts.end())
            count = it->second;


        if (count < required[c])
            return false;
    }


    return true;
}


// ============================================================
// Main BFS Fair + Coverage 2D
// ============================================================

optional<Result2D> bfsFairCov2D(
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


    if (n == 0)
        return nullopt;


    if (required.size()
        != raw.uniqueColorsList.size())
    {
        return nullopt;
    }


    // ========================================================
    // STEP 1:
    // Collect distinct X and Y coordinates
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


    int nx =
        static_cast<int>(
            xs.size()
        );

    int ny =
        static_cast<int>(
            ys.size()
        );


    // ========================================================
    // STEP 2:
    // Find initial query rectangle boundary indices
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


    if (qXL >= nx
        || qXR < 0
        || qYL >= ny
        || qYR < 0
        || qXL > qXR
        || qYL > qYR)
    {
        return nullopt;
    }


    // ========================================================
    // STEP 3:
    // Count |O|
    // ========================================================

    int count_O = 0;


    for (const auto &p : raw.pts) {

        if (insideRectangleCov(
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
    // STEP 4:
    // Priority queue and visited states
    // ========================================================

    priority_queue<
        RectStateCov,
        vector<RectStateCov>,
        RectCompareCov
    > pq;


    unordered_set<
        RectKeyCov,
        RectKeyHashCov
    > visited;


    // Initial query rectangle has similarity 1.

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
    // Helper for inserting neighboring rectangles
    // ========================================================

    auto pushNeighbor =
        [&](int xl,
            int xr,
            int yl,
            int yr)
    {
        if (xl < 0
            || xr >= nx
            || yl < 0
            || yr >= ny
            || xl > xr
            || yl > yr)
        {
            return;
        }


        RectKeyCov key{
            xl,
            xr,
            yl,
            yr
        };


        if (visited.find(key)
            != visited.end())
        {
            return;
        }


        visited.insert(key);


        int count_I = 0;
        int count_inter = 0;

        unordered_map<string, int>
            colorCounts;


        evaluateRectangleCov(
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


        if (count_I == 0)
            return;


        double similarity =
            tverskyByCount2D_Cov(
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
    // STEP 5:
    // Best-first search
    // ========================================================

    while (!pq.empty()) {

        RectStateCov current =
            pq.top();

        pq.pop();


        double x_min =
            xs[current.xl];

        double x_max =
            xs[current.xr];

        double y_min =
            ys[current.yl];

        double y_max =
            ys[current.yr];


        int count_I = 0;
        int count_inter = 0;

        unordered_map<string, int>
            colorCounts;


        evaluateRectangleCov(
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


        bool fair =
            isFairCounts2D_Cov(
                colorCounts
            );


        bool covered =
            coverageSatisfied2D_BFS(
                colorCounts,
                raw.uniqueColorsList,
                required
            );


        // Return first highest-similarity candidate
        // satisfying BOTH fairness and coverage.

        if (fair && covered) {

            return Result2D{
                x_min,
                x_max,
                y_min,
                y_max,
                current.similarity
            };
        }


        // ====================================================
        // Generate 8 neighboring rectangles
        // ====================================================


        // X-left expand

        pushNeighbor(
            current.xl - 1,
            current.xr,
            current.yl,
            current.yr
        );


        // X-left contract

        pushNeighbor(
            current.xl + 1,
            current.xr,
            current.yl,
            current.yr
        );


        // X-right expand

        pushNeighbor(
            current.xl,
            current.xr + 1,
            current.yl,
            current.yr
        );


        // X-right contract

        pushNeighbor(
            current.xl,
            current.xr - 1,
            current.yl,
            current.yr
        );


        // Y-lower expand

        pushNeighbor(
            current.xl,
            current.xr,
            current.yl - 1,
            current.yr
        );


        // Y-lower contract

        pushNeighbor(
            current.xl,
            current.xr,
            current.yl + 1,
            current.yr
        );


        // Y-upper expand

        pushNeighbor(
            current.xl,
            current.xr,
            current.yl,
            current.yr + 1
        );


        // Y-upper contract

        pushNeighbor(
            current.xl,
            current.xr,
            current.yl,
            current.yr - 1
        );
    }


    return nullopt;
}