#include "kd_tree_cov.h"

optional<Result> queryBestKD_cov(const Data &data, const KDTree &tree,
                                  const CoverageData &cov,
                                  int epsilon, double startVal, double endVal,
                                  double alpha, double beta,
                                  const vector<int> &required) {
    int n = data.n;
    if (n == 0) return nullopt;

    int L0 = lowerBound(data.keys, min(startVal, endVal));
    int R0 = upperBound(data.keys, max(startVal, endVal)) - 1;
    L0=max(L0,0); R0=min(R0,n-1);
    if (L0>R0) return nullopt;

    int qL=L0+1, qR=R0+1;
    double bestT=-1.0; int bestL=-1, bestR=-1;

    vector<double> lo(data.m+1), hi(data.m+1);

    for (int p=0; p<n; ++p) {
        // Step 1: find coverage point for this left boundary
        int coveragePoint = findCoveragePoint(cov, p, required, n);
        if (coveragePoint == -1) continue;

        // Step 2: find R closest to qR at or after coveragePoint that satisfies fairness
        const auto &center = data.points[p];
        for (int d=0; d<data.m; ++d) { lo[d]=center[d]-epsilon; hi[d]=center[d]+epsilon; }

        int bestRforP = -1;
        long long bestDist = LLONG_MAX;

        // Search outward from qR — but only consider R >= coveragePoint
        for (int delta=0; delta<=n; ++delta) {
            int rRight = qR + delta;
            if (rRight <= n && rRight >= coveragePoint) {
                lo[data.m] = hi[data.m] = (double)rRight;
                if (tree.pointExistsInRange(lo, hi)) {
                    long long dist = abs((long long)rRight - qR);
                    if (dist < bestDist || (dist==bestDist && rRight < bestRforP)) {
                        bestRforP=rRight; bestDist=dist;
                    }
                }
            }

            int rLeft = qR - delta;
            if (delta > 0 && rLeft >= coveragePoint) {
                lo[data.m] = hi[data.m] = (double)rLeft;
                if (tree.pointExistsInRange(lo, hi)) {
                    long long dist = abs((long long)rLeft - qR);
                    if (dist < bestDist || (dist==bestDist && rLeft < bestRforP)) {
                        bestRforP=rLeft; bestDist=dist;
                    }
                }
            }

            if (bestRforP != -1 && delta > bestDist) break;
            if (rRight > n && rLeft < coveragePoint) break;
        }

        if (bestRforP == -1) continue;

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
