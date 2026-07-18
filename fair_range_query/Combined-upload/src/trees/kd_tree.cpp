#include "kd_tree.h"

KDTree::KDTree(const vector<vector<double>> &points, int dims)
    : dims(dims), P(points) {
    int n = (int)P.size();
    vector<int> all(n); iota(all.begin(), all.end(), 0);
    root = build(all, 0, n, 0);
}

void KDTree::nthElement(vector<int> &a, int lo, int mid, int hi, int dim) {
    int l=lo, r=hi-1;
    while (true) {
        int i=l, j=r;
        double pivot = P[a[(l+r)>>1]][dim];
        while (i<=j) {
            while (P[a[i]][dim] < pivot) ++i;
            while (P[a[j]][dim] > pivot) --j;
            if (i<=j) { swap(a[i],a[j]); ++i; --j; }
        }
        if (j<mid) l=i; else r=j;
        if (l>=mid && r<=mid) return;
    }
}

unique_ptr<KDTree::Node> KDTree::build(vector<int> &idx, int loi, int hii, int depth) {
    auto node = make_unique<Node>();
    node->lo.assign(dims,  numeric_limits<double>::infinity());
    node->hi.assign(dims, -numeric_limits<double>::infinity());
    for (int k=loi; k<hii; ++k) {
        const auto &pt = P[idx[k]];
        for (int d=0; d<dims; ++d) { node->lo[d]=min(node->lo[d],pt[d]); node->hi[d]=max(node->hi[d],pt[d]); }
    }
    int n = hii-loi;
    if (n <= LEAF_CAP) {
        node->isLeaf = true;
        node->idxs.assign(idx.begin()+loi, idx.begin()+hii);
        return node;
    }
    node->dim = depth % dims;
    int mid = loi + n/2;
    nthElement(idx, loi, mid, hii, node->dim);
    node->split = P[idx[mid]][node->dim];
    node->left  = build(idx, loi, mid, depth+1);
    node->right = build(idx, mid, hii, depth+1);
    return node;
}

bool KDTree::overlaps(const Node *node, const vector<double> &qlo, const vector<double> &qhi) const {
    for (int d=0; d<dims; ++d)
        if (node->hi[d] < qlo[d] || node->lo[d] > qhi[d]) return false;
    return true;
}

bool KDTree::contains(const vector<double> &pt, const vector<double> &qlo, const vector<double> &qhi) const {
    for (int d=0; d<dims; ++d)
        if (pt[d] < qlo[d] || pt[d] > qhi[d]) return false;
    return true;
}

bool KDTree::search(const Node *node, const vector<double> &lo, const vector<double> &hi) const {
    if (!node || !overlaps(node, lo, hi)) return false;
    if (node->isLeaf) {
        for (int id : node->idxs) if (contains(P[id], lo, hi)) return true;
        return false;
    }
    return search(node->left.get(), lo, hi) || search(node->right.get(), lo, hi);
}

bool KDTree::pointExistsInRange(const vector<double> &lo, const vector<double> &hi) const {
    return search(root.get(), lo, hi);
}

void KDTree::closest(const Node *node, const vector<double> &qlo, const vector<double> &qhi,
                     int target, int &best, long long &bestDist) const {
    if (!node || !overlaps(node, qlo, qhi)) return;

    double minIdx = node->lo[dims - 1];
    double maxIdx = node->hi[dims - 1];
    long long minDist = 0;
    if (target < minIdx) minDist = (long long)minIdx - target;
    else if (target > maxIdx) minDist = target - (long long)maxIdx;

    if (minDist > bestDist) return;
    if (minDist == bestDist && best != -1 && minIdx > best) return;

    if (node->isLeaf) {
        for (int id : node->idxs) {
            if (contains(P[id], qlo, qhi)) {
                int ptIdx = (int)P[id][dims - 1];
                long long dist = abs((long long)ptIdx - target);
                if (dist < bestDist || (dist == bestDist && ptIdx < best)) {
                    bestDist = dist;
                    best = ptIdx;
                }
            }
        }
        return;
    }

    if (node->dim == dims - 1) {
        if (target <= node->split) {
            closest(node->left.get(), qlo, qhi, target, best, bestDist);
            closest(node->right.get(), qlo, qhi, target, best, bestDist);
        } else {
            closest(node->right.get(), qlo, qhi, target, best, bestDist);
            closest(node->left.get(), qlo, qhi, target, best, bestDist);
        }
    } else {
        closest(node->left.get(), qlo, qhi, target, best, bestDist);
        closest(node->right.get(), qlo, qhi, target, best, bestDist);
    }
}

int KDTree::findClosestIndexToTarget(const vector<double> &lo, const vector<double> &hi, int target) const {
    int best = -1;
    long long bestDist = LLONG_MAX;
    closest(root.get(), lo, hi, target, best, bestDist);
    return best;
}

optional<Result> queryBestKD(const Data &data, const KDTree &tree,
                             double startVal, double endVal,
                             double alpha, double beta) {
    auto search_start_time = std::chrono::high_resolution_clock::now();
    int n = data.n;
    if (n == 0) return nullopt;

    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0=max(L0,0); R0=min(R0,n-1);
    if (L0 > R0) return nullopt;

    int qL=L0+1, qR=R0+1;
    double bestT=-1.0; int bestL=-1, bestR=-1;

    vector<double> lo(data.m+1), hi(data.m+1);

    for (int p=0; p<n; ++p) {
        const auto &center = data.points[p];
        setQueryBounds(data, center, lo, hi);

        lo[data.m] = p + 1; hi[data.m] = n;
        auto [bestRforP_1, bestRforP_2] = getBothClosest(tree, lo, hi, qR, data.m);
        for (int bestRforP : {bestRforP_1, bestRforP_2}) {
            if (bestRforP == -1) continue;
        int L=p+1, R=bestRforP;
        double t = tversky(L, R, qL, qR, alpha, beta);

        if (t > bestT) {
        auto current_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(current_time - search_start_time).count();
        std::cout << "[Progress] Time: " << std::fixed << std::setprecision(4) << elapsed_ms << " ms, Similarity: " << t << "\n";
            bestT=t; bestL=L; bestR=R;
        } else if (t==bestT && t>=0.0) {
            int wBest=bestR-bestL, wCur=R-L;
            if (wCur<wBest||(wCur==wBest&&L<bestL)||(wCur==wBest&&L==bestL&&R<bestR)) { bestL=L; bestR=R; }
        }
        }
    }

    if (bestL==-1) return nullopt;
    return Result{data.keys[bestL-1], data.keys[bestR-1], bestT};
}
