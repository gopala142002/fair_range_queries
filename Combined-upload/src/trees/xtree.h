#pragma once
#include "../common/common.h"

// X-tree: extends R* tree by avoiding bad splits.
// When a split produces overlap above threshold, node grows into a supernode.
// Supernodes are scanned linearly during query but prevent cascading overlap.
struct XTree {
    static constexpr int    MAX_ENTRIES      = 4;
    static constexpr int    MIN_ENTRIES      = 2;
    static constexpr double OVERLAP_THRESHOLD = 0.0; // 0 = only split if zero overlap

    int totalDims;
    vector<vector<double>> &P;

    struct XNode {
        vector<double> mbrLo, mbrHi;
        vector<int> children;   // child node indices (internal)
        vector<int> pointIds;   // point indices (leaf)
        bool isLeaf    = false;
        bool isSupernode = false; // supernode: exceeds MAX_ENTRIES
        int  parent    = -1;
    };

    vector<XNode> nodes;
    int root = -1;

    XTree(vector<vector<double>> &points, int dims);

    int findClosestIndexToTarget(const vector<double> &lo,
                                 const vector<double> &hi, int target);

private:
    int    newNode();
    int    buildSTR(vector<int> &ptIds, int depth);
    void   computeMBR(int nIdx);
    void   getAllEntries(int nIdx, vector<int> &entries);

    // R* split helpers
    int    chooseSplitAxis(vector<int> &entries, bool isLeaf);
    int    chooseSplitIndex(vector<int> &entries, bool isLeaf, int axis);
    double computeOverlapAfterSplit(vector<int> &entries, bool isLeaf,
                                    int axis, int splitAt);

    double computeOverlap(const vector<double> &lo1, const vector<double> &hi1,
                          const vector<double> &lo2, const vector<double> &hi2);
    double computeArea(const vector<double> &lo, const vector<double> &hi);

    // Query helpers
    bool   overlapsBox(const XNode &node, const vector<double> &lo,
                       const vector<double> &hi);
    bool   pointInBox(int ptIdx, const vector<double> &lo,
                      const vector<double> &hi);
    double mindistToTarget(const XNode &node, int target);
};

optional<Result> queryBestXT(Data &data, XTree &tree,
                             double startVal, double endVal,
                             double alpha, double beta);
