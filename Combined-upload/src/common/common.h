#pragma once
#include <bits/stdc++.h>
#include "config.h"
using namespace std;

// Shared data structures, CSV parsing, binary search, and Tversky similarity.

struct Data {
    int n;
    int md; // number of difference dimensions (diffPairs.size())
    int mr; // number of ratio pairs (ratioPairs.size())
    int m;  // total tree dimensions (md + 2*mr + 1)
    
    vector<double> keys;
    vector<string> pairNames;
    set<string> uniqueColors;
    
    // prefix[c][t]: cumulative count of color c at row t
    // Note: color mappings are tracked by indices.
    vector<vector<int>> prefix; 
    
    // ratioPrefix[c][t]: weighted cumulative sum for color c at row t
    vector<vector<double>> ratioPrefix;
    
    // points[t]: unified point layout: [diff_dims..., ratio_dims..., index]
    // size is n+1, dimensions is m+1 (wait, m is already total dims in old code)
    // Let's keep m as total dims: m = md + 2*mr. So point dims = m + 1.
    vector<vector<double>> points; 
    
    double pointsBuildTimeMs = 0.0;
};

struct Result {
    double leftVal, rightVal, similarity;
};

inline vector<string> splitCSV(const string &line) {
    vector<string> out; string cur; bool inQ = false;
    for (char ch : line) {
        if (ch == '"') inQ = !inQ;
        else if (ch == ',' && !inQ) { out.push_back(cur); cur.clear(); }
        else cur += ch;
    }
    out.push_back(cur);
    return out;
}

inline optional<double> tryParse(const string &s) {
    try { size_t p; double v = stod(s, &p); return v; } catch (...) { return nullopt; }
}

inline double parseDoubleSafe(const string &s) {
    auto v = tryParse(s); return v ? *v : 0.0;
}

inline bool isPairName(const string &name) {
    auto d = name.find('-');
    return d != string::npos && d > 0 && d < name.size()-1 && name.find('-', d+1) == string::npos;
}

inline int lowerBound(const vector<double> &a, double x) {
    int lo=0, hi=(int)a.size();
    while (lo<hi) { int mid=(lo+hi)>>1; if (a[mid]<x) lo=mid+1; else hi=mid; }
    return lo;
}

inline int upperBound(const vector<double> &a, double x) {
    int lo=0, hi=(int)a.size();
    while (lo<hi) { int mid=(lo+hi)>>1; if (a[mid]<=x) lo=mid+1; else hi=mid; }
    return lo;
}

// Tversky index: T(I,O) = |I∩O| / (|I∩O| + alpha*|I\O| + beta*|O\I|)
inline double tversky(int aL, int aR, int bL, int bR, double alpha, double beta) {
    int interL = max(aL, bL), interR = min(aR, bR);
    double inter = (interL <= interR) ? (double)(interR - interL + 1) : 0.0;
    double onlyA = (double)(aR - aL + 1) - inter; // |I\O|
    double onlyB = (double)(bR - bL + 1) - inter; // |O\I|
    double denom = inter + alpha * onlyA + beta * onlyB;
    return (denom == 0.0) ? 0.0 : inter / denom;
}

template<typename Tree>
inline std::pair<int, int> getBothClosest(Tree& tree, std::vector<double>& lo, std::vector<double>& hi, int target, int index_dim) {
    double old_hi = hi[index_dim];
    hi[index_dim] = std::min(hi[index_dim], (double)target);
    int best1 = tree.findClosestIndexToTarget(lo, hi, target);
    hi[index_dim] = old_hi;
    
    double old_lo = lo[index_dim];
    lo[index_dim] = std::max(lo[index_dim], (double)target);
    int best2 = tree.findClosestIndexToTarget(lo, hi, target);
    lo[index_dim] = old_lo;
    
    return {best1, best2};
}



Data readCSV(const string &filename);
void setQueryBounds(const Data& data, const vector<double>& center, vector<double>& lo, vector<double>& hi, bool isRightSide = false);
bool isFair(const Data& data, int L, int R);
bool loadConstraintsFromConsole();
