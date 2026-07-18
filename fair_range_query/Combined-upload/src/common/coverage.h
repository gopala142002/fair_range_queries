#pragma once
#include "common.h"

// Per-color prefix sums and coverage point computation.
// No dataset changes needed — color column is already present.

struct CoverageData {
    vector<string> colorNames;           // ordered list of unique color names
    vector<vector<int>> colorPrefix;     // colorPrefix[c][t] = count of color c up to position t
    map<string, int> colorIndex;         // color name -> index in colorPrefix
};

CoverageData buildCoverageData(const string &filename,
                                const vector<string> &colorOrder);

// For a fixed left boundary p and required counts per color,
// find the minimum R such that range [p+1, R] contains >= required[c] of each color.
// Uses binary search on each color's prefix sum array.
// Returns -1 if no such R exists within [1, n].
int findCoveragePoint(const CoverageData &cov, int p,
                      const vector<int> &required, int n);
int findMaxLForCoverage(const CoverageData &cov, int r,
                        const vector<int> &required, int n);
