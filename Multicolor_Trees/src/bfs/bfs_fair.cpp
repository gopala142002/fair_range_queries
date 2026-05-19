#include "bfs_fair.h"

static double tverskByIndex(int aL, int aR, int bL, int bR,
                             double alpha, double beta) {
    int interL = max(aL, bL), interR = min(aR, bR);
    double inter = (interL <= interR) ? (double)(interR - interL + 1) : 0.0;
    double onlyA = (double)(aR - aL + 1) - inter;
    double onlyB = (double)(bR - bL + 1) - inter;
    double denom = inter + alpha * onlyA + beta * onlyB;
    return (denom == 0.0) ? 0.0 : inter / denom;
}

static long long packKey(int L, int R) {
    return (((long long)L) << 32) | (unsigned int)R;
}

static bool isFair(const CoverageData &cov, int L, int R, int epsilon) {
    int numColors = (int)cov.colorPrefix.size();
    int minCnt = INT_MAX, maxCnt = INT_MIN;
    for (int c = 0; c < numColors; ++c) {
        int cnt = cov.colorPrefix[c][R + 1] - cov.colorPrefix[c][L];
        minCnt = min(minCnt, cnt);
        maxCnt = max(maxCnt, cnt);
    }
    return (maxCnt - minCnt) <= epsilon;
}

static vector<pair<int,int>> getNeighbors(const double *keys, int n, int L, int R) {
    vector<pair<int,int>> nbrs;
    if (L < R) {
        double val = keys[L]; int i = L + 1;
        while (i <= R && keys[i] == val) ++i;
        if (i <= R) nbrs.push_back({i, R});
    }
    if (L > 0) {
        double val = keys[L]; int i = L - 1;
        while (i >= 0 && keys[i] == val) --i;
        if (i >= 0) nbrs.push_back({i, R});
    }
    if (R > L) {
        double val = keys[R]; int i = R - 1;
        while (i >= L && keys[i] == val) --i;
        if (i >= L) nbrs.push_back({L, i});
    }
    if (R < n - 1) {
        double val = keys[R]; int i = R + 1;
        while (i < n && keys[i] == val) ++i;
        if (i < n) nbrs.push_back({L, i});
    }
    return nbrs;
}

optional<BFSFairResult> bfsFair(
    const Data         &data,
    const CoverageData &cov,
    double startVal, double endVal,
    int    epsilon,
    double alpha, double beta)
{
    int n = data.n;
    if (n == 0) return nullopt;

    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0 = max(L0, 0); R0 = min(R0, n - 1);
    if (L0 > R0) return nullopt;

    using Entry = tuple<double, int, int>;
    auto cmp = [](const Entry &a, const Entry &b) {
        if (get<0>(a) != get<0>(b)) return get<0>(a) < get<0>(b);
        int wa = get<2>(a) - get<1>(a), wb = get<2>(b) - get<1>(b);
        if (wa != wb) return wa > wb;
        if (get<1>(a) != get<1>(b)) return get<1>(a) > get<1>(b);
        return get<2>(a) > get<2>(b);
    };
    priority_queue<Entry, vector<Entry>, decltype(cmp)> pq(cmp);

    unordered_set<long long> visited;
    double sim0 = tverskByIndex(L0, R0, L0, R0, alpha, beta);
    pq.push({sim0, L0, R0});
    visited.insert(packKey(L0, R0));

    const double *keys = data.keys.data();

    while (!pq.empty()) {
        auto [sim, L, R] = pq.top(); pq.pop();

        // Fairness check only — no coverage
        if (isFair(cov, L, R, epsilon)) {
            return BFSFairResult{data.keys[L], data.keys[R], sim};
        }

        for (auto [nL, nR] : getNeighbors(keys, n, L, R)) {
            long long key = packKey(nL, nR);
            if (visited.count(key)) continue;
            visited.insert(key);
            double nsim = tverskByIndex(L0, R0, nL, nR, alpha, beta);
            pq.push({nsim, nL, nR});
        }
    }
    return nullopt;
}
