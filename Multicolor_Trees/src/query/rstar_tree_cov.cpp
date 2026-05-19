#include "rstar_tree_cov.h"

optional<Result> queryBestRT_rstar_cov(Data &data, RStarTree &tree,
                                       const CoverageData &cov,
                                       int epsilon, double startVal, double endVal,
                                       double alpha, double beta,
                                       const vector<int> &required) {
    int n=data.n; if(n==0) return nullopt;
    int L0=lowerBound(data.keys,min(startVal,endVal));
    int R0=upperBound(data.keys,max(startVal,endVal))-1;
    L0=max(L0,0); R0=min(R0,n-1); if(L0>R0) return nullopt;
    int qL=L0+1, qR=R0+1;
    double bestT=-1.0; int bestL=-1, bestR=-1;
    vector<double> lo(data.m+1), hi(data.m+1);

    for(int p=0;p<n;++p){
        int coveragePoint=findCoveragePoint(cov,p,required,n);
        if(coveragePoint==-1) continue;
        const auto &center=data.points[p];
        for(int d=0;d<data.m;++d){lo[d]=center[d]-epsilon;hi[d]=center[d]+epsilon;}
        lo[data.m]=coveragePoint; hi[data.m]=n;
        int bestRforP=tree.findClosestIndexToTarget(lo,hi,qR);
        if(bestRforP==-1) continue;
        int L=p+1, R=bestRforP;
        double t=tversky(L,R,qL,qR,alpha,beta);
        if(t>bestT){bestT=t;bestL=L;bestR=R;}
        else if(t==bestT&&t>=0.0){
            int wBest=bestR-bestL,wCur=R-L;
            if(wCur<wBest||(wCur==wBest&&L<bestL)||(wCur==wBest&&L==bestL&&R<bestR))
                {bestL=L;bestR=R;}
        }
    }
    if(bestL==-1) return nullopt;
    return Result{data.keys[bestL-1],data.keys[bestR-1],bestT};
}
