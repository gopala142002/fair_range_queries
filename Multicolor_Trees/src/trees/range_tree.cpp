#include "range_tree.h"

RangeTree::RangeTree(vector<vector<double>> &points, int dims)
    : totalDims(dims), P(points) {
    int n = (int)P.size();
    vector<int> allIdx(n); iota(allIdx.begin(), allIdx.end(), 0);
    root = buildTree(allIdx, 0);
}

RangeTree::RTNode* RangeTree::newNode() {
    pool.emplace_back(); return &pool.back();
}

RangeTree::RTNode* RangeTree::buildTree(vector<int> idx, int dim) {
    if (idx.empty()) return nullptr;
    sort(idx.begin(), idx.end(), [&](int a, int b){ return P[a][dim] < P[b][dim]; });
    int mid = (int)idx.size()/2;
    RTNode *node = newNode();
    node->dim=dim; node->ptIdx=idx[mid]; node->splitVal=P[idx[mid]][dim];

    vector<int> leftIdx(idx.begin(), idx.begin()+mid);
    vector<int> rightIdx(idx.begin()+mid+1, idx.end());
    node->left  = buildTree(leftIdx,  dim);
    node->right = buildTree(rightIdx, dim);
    if (dim+1 < totalDims) node->nextTree = buildTree(idx, dim+1);
    return node;
}

bool RangeTree::pointInRange(int idx, const vector<double> &lo, const vector<double> &hi) {
    const auto &pt = P[idx];
    for (int d=0; d<totalDims; ++d) if (pt[d]<lo[d]||pt[d]>hi[d]) return false;
    return true;
}

RangeTree::RTNode* RangeTree::findSplitNode(RTNode *node, double lo, double hi) {
    RTNode *v=node;
    while (v) {
        if (!v->left && !v->right) break;
        if (hi < v->splitVal) v=v->left;
        else if (lo > v->splitVal) v=v->right;
        else break;
    }
    return v;
}

void RangeTree::updateBest(int ptIdx, int target, int &best, long long &bestDist) {
    double idxVal = P[ptIdx][totalDims-1];
    long long dist = llabs((long long)idxVal - target);
    if (dist < bestDist || (dist==bestDist && (int)idxVal < best)) { best=(int)idxVal; bestDist=dist; }
}

void RangeTree::closestInIndexTree(RTNode *node, double lo, double hi,
                                   int target, int &best, long long &bestDist) {
    if (!node) return;
    double idxVal = P[node->ptIdx][totalDims-1];
    if (idxVal>=lo && idxVal<=hi) {
        long long dist = llabs((long long)idxVal - target);
        if (dist<bestDist||(dist==bestDist&&(int)idxVal<best)) { best=(int)idxVal; bestDist=dist; }
    }
    if (target <= (int)idxVal) {
        closestInIndexTree(node->left,  lo, hi, target, best, bestDist);
        if (bestDist>0 && idxVal<=hi) closestInIndexTree(node->right, lo, hi, target, best, bestDist);
    } else {
        closestInIndexTree(node->right, lo, hi, target, best, bestDist);
        if (bestDist>0 && idxVal>=lo) closestInIndexTree(node->left,  lo, hi, target, best, bestDist);
    }
}

void RangeTree::queryClosest(RTNode *node, const vector<double> &lo, const vector<double> &hi,
                             int dim, int target, int &best, long long &bestDist) {
    if (!node) return;
    if (dim == totalDims-1) { closestInIndexTree(node, lo[dim], hi[dim], target, best, bestDist); return; }

    RTNode *split = findSplitNode(node, lo[dim], hi[dim]);
    if (!split) return;

    if (pointInRange(split->ptIdx, lo, hi)) updateBest(split->ptIdx, target, best, bestDist);

    RTNode *v = split->left;
    while (v) {
        if (lo[dim] <= v->splitVal) {
            if (v->right) { RTNode *sub = v->right->nextTree ? v->right->nextTree : v->right; queryClosest(sub, lo, hi, dim+1, target, best, bestDist); }
            if (pointInRange(v->ptIdx, lo, hi)) updateBest(v->ptIdx, target, best, bestDist);
            v=v->left;
        } else v=v->right;
    }

    v = split->right;
    while (v) {
        if (hi[dim] >= v->splitVal) {
            if (v->left) { RTNode *sub = v->left->nextTree ? v->left->nextTree : v->left; queryClosest(sub, lo, hi, dim+1, target, best, bestDist); }
            if (pointInRange(v->ptIdx, lo, hi)) updateBest(v->ptIdx, target, best, bestDist);
            v=v->right;
        } else v=v->left;
    }
}

int RangeTree::findClosestIndexToTarget(const vector<double> &lo, const vector<double> &hi, int target) {
    int best=-1; long long bestDist=LLONG_MAX;
    queryClosest(root, lo, hi, 0, target, best, bestDist);
    return best;
}

optional<Result> queryBestRT(Data &data, RangeTree &tree,
                             int epsilon, double startVal, double endVal,
                             double alpha, double beta) {
    int n=data.n;
    if (n==0) return nullopt;

    int L0=lowerBound(data.keys, min(startVal,endVal));
    int R0=upperBound(data.keys, max(startVal,endVal))-1;
    L0=max(L0,0); R0=min(R0,n-1);
    if (L0>R0) return nullopt;

    int qL=L0+1, qR=R0+1;
    double bestT=-1.0; int bestL=-1, bestR=-1;

    vector<double> lo(data.m+1), hi(data.m+1);
    for (int p=0; p<n; ++p) {
        const auto &center = data.points[p];
        for (int d=0; d<data.m; ++d) { lo[d]=center[d]-epsilon; hi[d]=center[d]+epsilon; }
        lo[data.m]=p+1; hi[data.m]=n;

        int bestRforP = tree.findClosestIndexToTarget(lo, hi, qR);
        if (bestRforP==-1) continue;

        int L=p+1, R=bestRforP;
        double t = tversky(L, R, qL, qR, alpha, beta);

        if (t > bestT) {
            bestT=t; bestL=L; bestR=R;
        } else if (t==bestT && t>=0.0) {
            int wBest=bestR-bestL, wCur=R-L;
            if (wCur<wBest||(wCur==wBest&&L<bestL)||(wCur==wBest&&L==bestL&&R<bestR)) { bestL=L; bestR=R; }
        }
    }

    if (bestL==-1) return nullopt;
    return Result{data.keys[bestL-1], data.keys[bestR-1], bestT};
}
