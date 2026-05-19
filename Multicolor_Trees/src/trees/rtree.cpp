#include "rtree.h"

RTree::RTree(vector<vector<double>> &points, int dims)
    : totalDims(dims), P(points) {
    int n = (int)P.size();
    vector<int> allIds(n);
    iota(allIds.begin(), allIds.end(), 0);
    root = buildNode(allIds);
}

// Build R-tree node recursively using Sort-Tile-Recursive (STR) packing.
// STR sorts points by each dimension in turn and groups into pages —
// produces well-packed MBRs with minimal overlap.
int RTree::buildNode(vector<int> &ptIds) {
    nodes.emplace_back();
    int idx = (int)nodes.size() - 1;

    if ((int)ptIds.size() <= MAX_ENTRIES) {
        // Leaf node
        nodes[idx].isLeaf  = true;
        nodes[idx].pointIds = ptIds;
        nodes[idx].mbrLo.assign(totalDims,  numeric_limits<double>::infinity());
        nodes[idx].mbrHi.assign(totalDims, -numeric_limits<double>::infinity());
        for (int pid : ptIds) {
            for (int d = 0; d < totalDims; ++d) {
                nodes[idx].mbrLo[d] = min(nodes[idx].mbrLo[d], P[pid][d]);
                nodes[idx].mbrHi[d] = max(nodes[idx].mbrHi[d], P[pid][d]);
            }
        }
        return idx;
    }

    // Internal node — split using STR on first dimension
    int dim = 0;
    sort(ptIds.begin(), ptIds.end(), [&](int a, int b) {
        return P[a][dim] < P[b][dim];
    });

    int total   = (int)ptIds.size();
    int groupSz = max(MAX_ENTRIES, (int)ceil((double)total / MAX_ENTRIES));

    nodes[idx].isLeaf = false;
    nodes[idx].mbrLo.assign(totalDims,  numeric_limits<double>::infinity());
    nodes[idx].mbrHi.assign(totalDims, -numeric_limits<double>::infinity());

    for (int start = 0; start < total; start += groupSz) {
        int end = min(start + groupSz, total);
        vector<int> chunk(ptIds.begin() + start, ptIds.begin() + end);
        int childIdx = buildNode(chunk);
        nodes[idx].children.push_back(childIdx);
        // Expand MBR to cover child
        for (int d = 0; d < totalDims; ++d) {
            nodes[idx].mbrLo[d] = min(nodes[idx].mbrLo[d], nodes[childIdx].mbrLo[d]);
            nodes[idx].mbrHi[d] = max(nodes[idx].mbrHi[d], nodes[childIdx].mbrHi[d]);
        }
    }
    return idx;
}

void RTree::updateMBR(RNode &node) {
    node.mbrLo.assign(totalDims,  numeric_limits<double>::infinity());
    node.mbrHi.assign(totalDims, -numeric_limits<double>::infinity());
    if (node.isLeaf) {
        for (int pid : node.pointIds)
            for (int d = 0; d < totalDims; ++d) {
                node.mbrLo[d] = min(node.mbrLo[d], P[pid][d]);
                node.mbrHi[d] = max(node.mbrHi[d], P[pid][d]);
            }
    } else {
        for (int c : node.children)
            for (int d = 0; d < totalDims; ++d) {
                node.mbrLo[d] = min(node.mbrLo[d], nodes[c].mbrLo[d]);
                node.mbrHi[d] = max(node.mbrHi[d], nodes[c].mbrHi[d]);
            }
    }
}

bool RTree::overlapsBox(const RNode &node, const vector<double> &lo,
                        const vector<double> &hi) {
    for (int d = 0; d < totalDims; ++d)
        if (node.mbrHi[d] < lo[d] || node.mbrLo[d] > hi[d]) return false;
    return true;
}

bool RTree::pointInBox(int ptIdx, const vector<double> &lo,
                       const vector<double> &hi) {
    for (int d = 0; d < totalDims; ++d)
        if (P[ptIdx][d] < lo[d] || P[ptIdx][d] > hi[d]) return false;
    return true;
}

// Minimum possible distance from any point in node to target on index dim
double RTree::mindistToTarget(const RNode &node, int target) {
    int idxDim = totalDims - 1;
    double lo  = node.mbrLo[idxDim];
    double hi  = node.mbrHi[idxDim];
    if (target < lo) return lo - target;
    if (target > hi) return target - hi;
    return 0.0;
}

int RTree::findClosestIndexToTarget(const vector<double> &lo,
                                    const vector<double> &hi, int target) {
    if (root == -1) return -1;

    // Best First Search: priority queue ordered by mindist to target on index dim
    // pair: (mindist, nodeIdx)
    using Entry = pair<double, int>;
    priority_queue<Entry, vector<Entry>, greater<Entry>> pq;

    if (overlapsBox(nodes[root], lo, hi))
        pq.push({mindistToTarget(nodes[root], target), root});

    int   best     = -1;
    double bestDist = numeric_limits<double>::infinity();

    while (!pq.empty()) {
        auto [dist, nIdx] = pq.top(); pq.pop();

        // Prune: if this node's mindist >= bestDist, no improvement possible
        if (dist >= bestDist) break;

        const RNode &node = nodes[nIdx];

        if (node.isLeaf) {
            for (int pid : node.pointIds) {
                if (!pointInBox(pid, lo, hi)) continue;
                double idxVal = P[pid][totalDims - 1];
                double d = abs(idxVal - target);
                if (d < bestDist || (d == bestDist && (int)idxVal < best)) {
                    best     = (int)idxVal;
                    bestDist = d;
                }
            }
        } else {
            for (int c : node.children) {
                if (!overlapsBox(nodes[c], lo, hi)) continue;
                double md = mindistToTarget(nodes[c], target);
                if (md < bestDist)
                    pq.push({md, c});
            }
        }
    }
    return best;
}

optional<Result> queryBestRT_rtree(Data &data, RTree &tree,
                                   int epsilon, double startVal, double endVal,
                                   double alpha, double beta) {
    int n = data.n;
    if (n == 0) return nullopt;

    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0 = max(L0, 0); R0 = min(R0, n - 1);
    if (L0 > R0) return nullopt;

    int qL = L0 + 1, qR = R0 + 1;
    double bestT = -1.0;
    int bestL = -1, bestR = -1;

    vector<double> lo(data.m + 1), hi(data.m + 1);

    for (int p = 0; p < n; ++p) {
        const auto &center = data.points[p];
        for (int d = 0; d < data.m; ++d) {
            lo[d] = center[d] - epsilon;
            hi[d] = center[d] + epsilon;
        }
        lo[data.m] = p + 1;
        hi[data.m] = n;

        int bestRforP = tree.findClosestIndexToTarget(lo, hi, qR);
        if (bestRforP == -1) continue;

        int L = p + 1, R = bestRforP;
        double t = tversky(L, R, qL, qR, alpha, beta);

        if (t > bestT) {
            bestT = t; bestL = L; bestR = R;
        } else if (t == bestT && t >= 0.0) {
            int wBest = bestR - bestL, wCur = R - L;
            if (wCur < wBest || (wCur == wBest && L < bestL) ||
                (wCur == wBest && L == bestL && R < bestR)) {
                bestL = L; bestR = R;
            }
        }
    }

    if (bestL == -1) return nullopt;
    return Result{data.keys[bestL - 1], data.keys[bestR - 1], bestT};
}
