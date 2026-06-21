#pragma once
#include "../common/common.h"

// R-tree with Best First Search for closest index to target within epsilon box.
struct RTree {
    static constexpr int MAX_ENTRIES = 4;  // max children per node
    static constexpr int MIN_ENTRIES = 2;  // min children per node

    int totalDims;
    vector<vector<double>> &P;

    struct RNode {
        vector<double> mbrLo, mbrHi;  // MBR bounds for all dims
        vector<int> children;          // indices into nodes[] (internal)
        vector<int> pointIds;          // point indices (leaf)
        bool isLeaf = false;
    };

    vector<RNode> nodes;
    int root = -1;

    RTree(vector<vector<double>> &points, int dims);

    // Find index-dim value closest to target within epsilon box [lo,hi]
    // Returns -1 if no valid point found
    int findClosestIndexToTarget(const vector<double> &lo,
                                 const vector<double> &hi, int target);

private:
    int buildNode(vector<int> &ptIds);
    void updateMBR(RNode &node);
    bool overlapsBox(const RNode &node, const vector<double> &lo,
                     const vector<double> &hi);
    bool pointInBox(int ptIdx, const vector<double> &lo,
                    const vector<double> &hi);
    double mindistToTarget(const RNode &node, int target);
};

optional<Result> queryBestRT_rtree(const Data &data, RTree &tree,
                                   double startVal, double endVal,
                                   double alpha, double beta);
