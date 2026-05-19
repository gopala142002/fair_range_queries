#pragma once
#include <bits/stdc++.h>
using namespace std;

// Shared data structures, CSV parsing, binary search, and Tversky similarity.

struct Data {
    int n, m;
    vector<double> keys;
    vector<string> pairNames;
    set<string> uniqueColors;
    vector<vector<int>>    prefix; // prefix[d][t]: cumulative color-pair value at row t
    vector<vector<double>> points; // points[t] = (prefix[0][t],...,prefix[m-1][t], t)
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

Data readCSV(const string &filename);
