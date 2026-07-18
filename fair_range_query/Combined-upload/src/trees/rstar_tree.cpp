#include "rstar_tree.h"

// R* tree using bulk STR loading + R* split strategy.
// Avoids recursive reinsertion which causes segfaults on large datasets.
// The key R* improvement over R-tree is the split strategy:
// chooses split axis minimizing margin sum, split index minimizing overlap.

RStarTree::RStarTree(vector<vector<double>> &points, int dims)
    : totalDims(dims), P(points) {
    nodes.reserve(points.size() * 4 + 16);
    int n = (int)P.size();
    vector<int> allIds(n);
    iota(allIds.begin(), allIds.end(), 0);
    root = buildSTR(allIds, 0);
}

int RStarTree::newNode() {
    if ((int)nodes.size() >= (int)nodes.capacity() - 4)
        nodes.reserve(nodes.capacity() * 2);
    nodes.push_back(RSNode());
    int idx = (int)nodes.size() - 1;
    nodes[idx].mbrLo.assign(totalDims,  numeric_limits<double>::infinity());
    nodes[idx].mbrHi.assign(totalDims, -numeric_limits<double>::infinity());
    nodes[idx].parent = -1;
    return idx;
}

// STR bulk loading: sort by dimension 0, group into pages,
// then recursively build with R* split strategy on overflow.
int RStarTree::buildSTR(vector<int> &ptIds, int depth) {
    if (ptIds.empty()) return -1;

    int nIdx = newNode();

    if ((int)ptIds.size() <= MAX_ENTRIES) {
        nodes[nIdx].isLeaf   = true;
        nodes[nIdx].pointIds = ptIds;
        computeMBR(nIdx);
        return nIdx;
    }

    // Sort by current dimension (cycle through dims for better packing)
    int dim = depth % totalDims;
    sort(ptIds.begin(), ptIds.end(), [&](int a, int b) {
        return P[a][dim] < P[b][dim];
    });

    nodes[nIdx].isLeaf = false;

    int total   = (int)ptIds.size();
    int groupSz = max(MAX_ENTRIES, (int)ceil((double)total / MAX_ENTRIES));

    for (int start = 0; start < total; start += groupSz) {
        int end = min(start + groupSz, total);
        vector<int> chunk(ptIds.begin() + start, ptIds.begin() + end);
        int childIdx = buildSTR(chunk, depth + 1);
        if (childIdx != -1) {
            nodes[childIdx].parent = nIdx;
            nodes[nIdx].children.push_back(childIdx);
        }
    }
    computeMBR(nIdx);
    return nIdx;
}

void RStarTree::computeMBR(int nIdx) {
    nodes[nIdx].mbrLo.assign(totalDims,  numeric_limits<double>::infinity());
    nodes[nIdx].mbrHi.assign(totalDims, -numeric_limits<double>::infinity());
    if (nodes[nIdx].isLeaf) {
        for (int pid : nodes[nIdx].pointIds)
            for (int d = 0; d < totalDims; ++d) {
                nodes[nIdx].mbrLo[d] = min(nodes[nIdx].mbrLo[d], P[pid][d]);
                nodes[nIdx].mbrHi[d] = max(nodes[nIdx].mbrHi[d], P[pid][d]);
            }
    } else {
        for (int c : nodes[nIdx].children)
            for (int d = 0; d < totalDims; ++d) {
                nodes[nIdx].mbrLo[d] = min(nodes[nIdx].mbrLo[d], nodes[c].mbrLo[d]);
                nodes[nIdx].mbrHi[d] = max(nodes[nIdx].mbrHi[d], nodes[c].mbrHi[d]);
            }
    }
}

void RStarTree::getAllEntries(int nIdx, vector<int> &entries) {
    if (nodes[nIdx].isLeaf) entries = nodes[nIdx].pointIds;
    else                    entries = nodes[nIdx].children;
}

double RStarTree::computeOverlap(const vector<double> &lo1, const vector<double> &hi1,
                                  const vector<double> &lo2, const vector<double> &hi2) {
    double overlap = 1.0;
    for (int d = 0; d < totalDims; ++d) {
        double inter = min(hi1[d],hi2[d]) - max(lo1[d],lo2[d]);
        if (inter <= 0) return 0.0;
        overlap *= inter;
    }
    return overlap;
}

double RStarTree::computeArea(const vector<double> &lo, const vector<double> &hi) {
    double area = 1.0;
    for (int d = 0; d < totalDims; ++d) {
        double ext = hi[d] - lo[d];
        if (ext <= 0) return 0.0;
        area *= ext;
    }
    return area;
}

// R* split axis: choose axis minimizing sum of margin values
int RStarTree::chooseSplitAxis(int nIdx) {
    vector<int> entries;
    getAllEntries(nIdx, entries);
    bool isLeaf = nodes[nIdx].isLeaf;
    int total   = (int)entries.size();
    int bestAxis = 0;
    double bestMarginSum = numeric_limits<double>::infinity();

    for (int axis = 0; axis < totalDims; ++axis) {
        sort(entries.begin(), entries.end(), [&](int a, int b) {
            double va = isLeaf ? P[a][axis] : nodes[a].mbrLo[axis];
            double vb = isLeaf ? P[b][axis] : nodes[b].mbrLo[axis];
            return va < vb;
        });
        double marginSum = 0;
        for (int k = 0; k <= total - 2*MIN_ENTRIES; ++k) {
            int split = MIN_ENTRIES + k;
            vector<double> lo1(totalDims, numeric_limits<double>::infinity());
            vector<double> hi1(totalDims,-numeric_limits<double>::infinity());
            vector<double> lo2(totalDims, numeric_limits<double>::infinity());
            vector<double> hi2(totalDims,-numeric_limits<double>::infinity());
            for (int i=0;     i<split; ++i) {
                int e=entries[i];
                for (int d=0;d<totalDims;++d){
                    double v=isLeaf?P[e][d]:nodes[e].mbrLo[d];
                    double w=isLeaf?P[e][d]:nodes[e].mbrHi[d];
                    lo1[d]=min(lo1[d],v); hi1[d]=max(hi1[d],w);
                }
            }
            for (int i=split; i<total; ++i) {
                int e=entries[i];
                for (int d=0;d<totalDims;++d){
                    double v=isLeaf?P[e][d]:nodes[e].mbrLo[d];
                    double w=isLeaf?P[e][d]:nodes[e].mbrHi[d];
                    lo2[d]=min(lo2[d],v); hi2[d]=max(hi2[d],w);
                }
            }
            for (int d=0;d<totalDims;++d)
                marginSum += (hi1[d]-lo1[d]) + (hi2[d]-lo2[d]);
        }
        if (marginSum < bestMarginSum) { bestMarginSum=marginSum; bestAxis=axis; }
    }
    return bestAxis;
}

// R* split index: minimize overlap, then area
int RStarTree::chooseSplitIndex(int nIdx, int axis) {
    vector<int> entries;
    getAllEntries(nIdx, entries);
    bool isLeaf = nodes[nIdx].isLeaf;
    int total   = (int)entries.size();

    sort(entries.begin(), entries.end(), [&](int a, int b) {
        double va = isLeaf ? P[a][axis] : nodes[a].mbrLo[axis];
        double vb = isLeaf ? P[b][axis] : nodes[b].mbrLo[axis];
        return va < vb;
    });

    int bestK=0;
    double bestOverlap=numeric_limits<double>::infinity();
    double bestArea   =numeric_limits<double>::infinity();

    for (int k=0; k<=total-2*MIN_ENTRIES; ++k) {
        int split=MIN_ENTRIES+k;
        vector<double> lo1(totalDims, numeric_limits<double>::infinity());
        vector<double> hi1(totalDims,-numeric_limits<double>::infinity());
        vector<double> lo2(totalDims, numeric_limits<double>::infinity());
        vector<double> hi2(totalDims,-numeric_limits<double>::infinity());
        for (int i=0;     i<split; ++i) {
            int e=entries[i];
            for (int d=0;d<totalDims;++d){
                double v=isLeaf?P[e][d]:nodes[e].mbrLo[d];
                double w=isLeaf?P[e][d]:nodes[e].mbrHi[d];
                lo1[d]=min(lo1[d],v); hi1[d]=max(hi1[d],w);
            }
        }
        for (int i=split; i<total; ++i) {
            int e=entries[i];
            for (int d=0;d<totalDims;++d){
                double v=isLeaf?P[e][d]:nodes[e].mbrLo[d];
                double w=isLeaf?P[e][d]:nodes[e].mbrHi[d];
                lo2[d]=min(lo2[d],v); hi2[d]=max(hi2[d],w);
            }
        }
        double overlap=computeOverlap(lo1,hi1,lo2,hi2);
        double area   =computeArea(lo1,hi1)+computeArea(lo2,hi2);
        if (overlap<bestOverlap||(overlap==bestOverlap&&area<bestArea))
            { bestOverlap=overlap; bestArea=area; bestK=k; }
    }
    return bestK;
}

bool RStarTree::overlapsBox(const RSNode &node, const vector<double> &lo,
                             const vector<double> &hi) {
    for (int d=0;d<totalDims;++d)
        if (node.mbrHi[d]<lo[d]||node.mbrLo[d]>hi[d]) return false;
    return true;
}

bool RStarTree::pointInBox(int ptIdx, const vector<double> &lo,
                            const vector<double> &hi) {
    for (int d=0;d<totalDims;++d)
        if (P[ptIdx][d]<lo[d]||P[ptIdx][d]>hi[d]) return false;
    return true;
}

double RStarTree::mindistToTarget(const RSNode &node, int target) {
    double lo=node.mbrLo[totalDims-1], hi=node.mbrHi[totalDims-1];
    if (target < lo) return lo-target;
    if (target > hi) return target-hi;
    return 0.0;
}

int RStarTree::findClosestIndexToTarget(const vector<double> &lo,
                                         const vector<double> &hi, int target) {
    if (root==-1) return -1;
    using Entry=pair<double,int>;
    priority_queue<Entry,vector<Entry>,greater<Entry>> pq;
    if (overlapsBox(nodes[root],lo,hi))
        pq.push({mindistToTarget(nodes[root],target),root});

    int    best    =-1;
    double bestDist=numeric_limits<double>::infinity();

    while (!pq.empty()) {
        auto [dist,nIdx]=pq.top(); pq.pop();
        if (dist>=bestDist) break;
        if (nodes[nIdx].isLeaf) {
            for (int pid:nodes[nIdx].pointIds) {
                if (!pointInBox(pid,lo,hi)) continue;
                double idxVal=P[pid][totalDims-1];
                double d=abs(idxVal-target);
                if (d<bestDist||(d==bestDist&&(int)idxVal<best))
                    { best=(int)idxVal; bestDist=d; }
            }
        } else {
            for (int c:nodes[nIdx].children) {
                if (!overlapsBox(nodes[c],lo,hi)) continue;
                double md=mindistToTarget(nodes[c],target);
                if (md<bestDist) pq.push({md,c});
            }
        }
    }
    return best;
}

optional<Result> queryBestRT_rstar(Data &data, RStarTree &tree,
                                   double startVal, double endVal,
                                   double alpha, double beta) {
    auto search_start_time = std::chrono::high_resolution_clock::now();
    int n=data.n; if(n==0) return nullopt;
    int L0=lowerBound(data.keys,min(startVal,endVal));
    int R0=upperBound(data.keys,max(startVal,endVal))-1;
    L0=max(L0,0); R0=min(R0,n-1); if(L0>R0) return nullopt;
    int qL=L0+1, qR=R0+1;
    double bestT=-1.0; int bestL=-1, bestR=-1;
    vector<double> lo(data.m+1), hi(data.m+1);

    for (int p = 0; p < n; ++p) {
        const auto &center = data.points[p];
        setQueryBounds(data, center, lo, hi);
        lo[data.m]=p+1; hi[data.m]=n;
        auto [bestRforP_1, bestRforP_2] = getBothClosest(tree, lo, hi, qR, data.m);
        for (int bestRforP : {bestRforP_1, bestRforP_2}) {
            if (bestRforP == -1) continue;
        int L=p+1, R=bestRforP;
        double t=tversky(L,R,qL,qR,alpha,beta);
        if (t > bestT) {
        auto current_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(current_time - search_start_time).count();
        std::cout << "[Progress] Time: " << std::fixed << std::setprecision(4) << elapsed_ms << " ms, Similarity: " << t << "\n";bestT=t;bestL=L;bestR=R;}
        else if(t==bestT&&t>=0.0){
            int wBest=bestR-bestL,wCur=R-L;
            if(wCur<wBest||(wCur==wBest&&L<bestL)||(wCur==wBest&&L==bestL&&R<bestR))
                {bestL=L;bestR=R;}
        }
        }
    }
    if(bestL==-1) return nullopt;
    return Result{data.keys[bestL-1],data.keys[bestR-1],bestT};
}
