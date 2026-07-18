#include "rstar_tree_opt.h"

static void updateResult(int L, int R, int qL, int qR,
                         double alpha, double beta,
                         double &bestT, int &bestL, int &bestR, const std::chrono::high_resolution_clock::time_point& search_start_time) {
    double t = tversky(L, R, qL, qR, alpha, beta);
    if (t > bestT) {
        auto current_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(current_time - search_start_time).count();
        std::cout << "[Progress] Time: " << std::fixed << std::setprecision(4) << elapsed_ms << " ms, Similarity: " << t << "\n"; bestT=t; bestL=L; bestR=R; }
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

    auto tryLeft=[&](int p){
        const auto &center=data.points[p];
        setQueryBounds(data, center, lo, hi);
        lo[data.m]=p+1; hi[data.m]=n;
        auto [bestRforP_1, bestRforP_2] = getBothClosest(tree, lo, hi, qR, data.m);
        if (bestRforP_1 != -1) { int R = bestRforP_1; updateResult(p+1,R,qL,qR,alpha,beta,bestT,bestL,bestR, search_start_time); }
        if (bestRforP_2 != -1) { int R = bestRforP_2; updateResult(p+1,R,qL,qR,alpha,beta,bestT,bestL,bestR, search_start_time); }
        //
    };
    auto tryRight=[&](int r){
        const auto &center=data.points[r];
        setQueryBounds(data, center, lo, hi, true);
        lo[data.m]=0; hi[data.m]=r-1;
        auto [bestLforR_1, bestLforR_2] = getBothClosest(tree, lo, hi, qL - 1, data.m);
        if (bestLforR_1 != -1) { int L = bestLforR_1 + 1; updateResult(L,r,qL,qR,alpha,beta,bestT,bestL,bestR, search_start_time); }
        if (bestLforR_2 != -1) { int L = bestLforR_2 + 1; updateResult(L,r,qL,qR,alpha,beta,bestT,bestL,bestR, search_start_time); }
        //
    };

    if(qR<=n-qL){
        for(int p=qL-2;p>=0;--p){if(ubLP1(p+1,qL,qR,alpha)<=bestT)break;tryLeft(p);}
        for(int p=qL-1;p<=qR-1;++p){if(ubLP2(p+1,qL,qR,beta)<=bestT)break;tryLeft(p);}
    } else {
        for(int r=qR;r<=n;++r){if(ubRP1(r,qL,qR,alpha)<=bestT)break;tryRight(r);}
        for(int r=qR-1;r>=qL;--r){if(ubRP2(r,qL,qR,beta)<=bestT)break;tryRight(r);}
    }
    if(bestL==-1) return nullopt;
    return Result{data.keys[bestL-1],data.keys[bestR-1],bestT};
}
