#include "rstar_tree_opt.h"

static void updateResult(int L, int R, int qL, int qR,
                         double alpha, double beta,
                         double &bestT, int &bestL, int &bestR) {
    double t = tversky(L, R, qL, qR, alpha, beta);
    if (t > bestT) { bestT=t; bestL=L; bestR=R; }
    else if (t==bestT && t>=0.0) {
        int wBest=bestR-bestL, wCur=R-L;
        if (wCur<wBest||(wCur==wBest&&L<bestL)||(wCur==wBest&&L==bestL&&R<bestR))
            { bestL=L; bestR=R; }
    }
}
static double ubLP1(int L,int qL,int qR,double b){double Q=qR-qL+1;return Q/(Q+b*(qL-L));}
static double ubLP2(int L,int qL,int qR,double a){double i=qR-L+1;return i/(i+a*(L-qL));}
static double ubRP1(int R,int qL,int qR,double b){double Q=qR-qL+1;return Q/(Q+b*(R-qR));}
static double ubRP2(int R,int qL,int qR,double a){double i=R-qL+1;return i/(i+a*(qR-R));}

optional<Result> queryBestRT_rstar_opt(Data &data, RStarTree &tree,
                                       int epsilon, double startVal, double endVal,
                                       double alpha, double beta) {
    int n=data.n; if(n==0) return nullopt;
    int L0=lowerBound(data.keys,min(startVal,endVal));
    int R0=upperBound(data.keys,max(startVal,endVal))-1;
    L0=max(L0,0); R0=min(R0,n-1); if(L0>R0) return nullopt;
    int qL=L0+1, qR=R0+1;
    double bestT=-1.0; int bestL=-1, bestR=-1;
    vector<double> lo(data.m+1), hi(data.m+1);

    auto tryLeft=[&](int p){
        const auto &center=data.points[p];
        for(int d=0;d<data.m;++d){lo[d]=center[d]-epsilon;hi[d]=center[d]+epsilon;}
        lo[data.m]=p+1; hi[data.m]=n;
        int R=tree.findClosestIndexToTarget(lo,hi,qR); if(R==-1) return;
        updateResult(p+1,R,qL,qR,alpha,beta,bestT,bestL,bestR);
    };
    auto tryRight=[&](int r){
        const auto &center=data.points[r];
        for(int d=0;d<data.m;++d){lo[d]=center[d]-epsilon;hi[d]=center[d]+epsilon;}
        lo[data.m]=1; hi[data.m]=r-1;
        int L=tree.findClosestIndexToTarget(lo,hi,qL); if(L==-1) return;
        updateResult(L,r,qL,qR,alpha,beta,bestT,bestL,bestR);
    };

    if(qR<=n-qL){
        for(int p=qL-2;p>=0;--p){if(ubLP1(p+1,qL,qR,beta)<=bestT)break;tryLeft(p);}
        for(int p=qL-1;p<=qR-1;++p){if(ubLP2(p+1,qL,qR,alpha)<=bestT)break;tryLeft(p);}
    } else {
        for(int r=qR;r<=n;++r){if(ubRP1(r,qL,qR,beta)<=bestT)break;tryRight(r);}
        for(int r=qR-1;r>=qL;--r){if(ubRP2(r,qL,qR,alpha)<=bestT)break;tryRight(r);}
    }
    if(bestL==-1) return nullopt;
    return Result{data.keys[bestL-1],data.keys[bestR-1],bestT};
}
