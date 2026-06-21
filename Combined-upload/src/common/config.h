#pragma once
#include <string>
#include <vector>

using namespace std;

// Configuration for difference constraints.
// For each pair, a dimension is added to the tree: (weightA*prefix[colorA] - weightB*prefix[colorB]).
// Query bound: [center - epsilon, center + epsilon]
struct DiffPairConfig {
    string colorA, colorB;
    double weightA, weightB;
    double epsilon;
};

// Configuration for ratio constraints.
// For each pair, TWO dimensions are added to the tree: cr and cb.
// cr = weightA * cumA - weightB * cumB * (1 + delta)
// cb = weightB * cumB - weightA * cumA * (1 + delta)
// Query bound: [-INF, value_at_{L-1}] for both
struct RatioPairConfig {
    string colorA, colorB;
    double weightA, weightB;
    double delta;
};

// Global configuration vectors (defined in common.cpp)
extern vector<DiffPairConfig> diffPairs;
extern vector<RatioPairConfig> ratioPairs;
