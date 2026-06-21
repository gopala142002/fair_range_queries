#include "bfs_fair_cov.h"

// ---- Tversky on index intervals ----
static double tverskByIndex(int aL, int aR, int bL, int bR,
                             double alpha, double beta) {
    int interL = max(aL, bL), interR = min(aR, bR);
    double inter  = (interL <= interR) ? (double)(interR - interL + 1) : 0.0;
    double onlyA  = (double)(aR - aL + 1) - inter;  // |I\O|
    double onlyB  = (double)(bR - bL + 1) - inter;  // |O\I|
    double denom  = inter + alpha * onlyA + beta * onlyB;
    return (denom == 0.0) ? 0.0 : inter / denom;
}

// ---- Pack [L,R] into a single 64-bit key for visited set ----
static long long packKey(int L, int R) {
    return (((long long)L) << 32) | (unsigned int)R;
}

// Use isFair from common.h
// ---- Coverage check: each color >= required[c] ----
static bool coversSatisfied(const CoverageData &cov, int L, int R,
                             const vector<int> &required) {
    int numColors = (int)cov.colorPrefix.size();
    for (int c = 0; c < numColors; ++c) {
        int cnt = cov.colorPrefix[c][R + 1] - cov.colorPrefix[c][L];
        if (cnt < required[c]) return false;
    }
    return true;
}

// ---- Neighbor generation: move each boundary by one distinct step ----
// Returns up to 4 neighbors [nL, nR]
static vector<pair<int,int>> getNeighbors(const double *keys, int n, int L, int R) {
    vector<pair<int,int>> nbrs;

    // Shrink left (move L right to next distinct value)
    if (L < R) {
        double val = keys[L];
        int i = L + 1;
        while (i <= R && keys[i] == val) ++i;
        if (i <= R) nbrs.push_back({i, R});
    }

    // Expand left (move L left to previous distinct value)
    if (L > 0) {
        double val = keys[L];
        int i = L - 1;
        while (i >= 0 && keys[i] == val) --i;
        if (i >= 0) nbrs.push_back({i, R});
    }

    // Shrink right (move R left to previous distinct value)
    if (R > L) {
        double val = keys[R];
        int i = R - 1;
        while (i >= L && keys[i] == val) --i;
        if (i >= L) nbrs.push_back({L, i});
    }

    // Expand right (move R right to next distinct value)
    if (R < n - 1) {
        double val = keys[R];
        int i = R + 1;
        while (i < n && keys[i] == val) ++i;
        if (i < n) nbrs.push_back({L, i});
    }

    return nbrs;
}

optional<BFSResult> bfsFairCov(
    const Data         &data,
    const CoverageData &cov,
    double startVal, double endVal,
    double alpha, double beta,
    const vector<int>  &required)
{
    int n = data.n;
    if (n == 0) return nullopt;

    // Map user range to index interval [L0, R0]
    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0 = max(L0, 0); R0 = min(R0, n - 1);
    if (L0 > R0) return nullopt;

    // Max-heap: (tversky, L, R) — always pop highest Tversky first
    using Entry = tuple<double, int, int>;
    auto cmp = [](const Entry &a, const Entry &b) {
        // max-heap: higher tversky first
        if (get<0>(a) != get<0>(b)) return get<0>(a) < get<0>(b);
        // tiebreak: smaller width
        int wa = get<2>(a) - get<1>(a);
        int wb = get<2>(b) - get<1>(b);
        if (wa != wb) return wa > wb;
        // tiebreak: smaller L
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

        // Check BOTH fairness AND coverage
        if (::isFair(data, L + 1, R + 1) && coversSatisfied(cov, L, R, required)) {
            return BFSResult{data.keys[L], data.keys[R], sim};
        }

        // Generate neighbours
        for (auto [nL, nR] : getNeighbors(keys, n, L, R)) {
            long long key = packKey(nL, nR);
            if (visited.count(key)) continue;
            visited.insert(key);
            double nsim = tverskByIndex(nL, nR, L0, R0, alpha, beta);
            pq.push({nsim, nL, nR});
        }
    }

    return nullopt; // no fair+coverage range found
}
