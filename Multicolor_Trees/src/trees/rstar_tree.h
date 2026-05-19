#pragma once
#include "../common/common.h"

// R* tree — STR bulk loading with R* split strategy (minimize overlap + margin).
// Key difference from R-tree: split chooses best axis (min margin sum) and
// best split index (min overlap, then min area) — produces less overlapping MBRs.
// Query interface identical to RTree.
struct RStarTree {
    static constexpr int MAX_ENTRIES = 4;
    static constexpr int MIN_ENTRIES = 2;

    int totalDims;
    vector<vector<double>> &P;

    struct RSNode {
        vector<double> mbrLo, mbrHi;
        vector<int> children;
        vector<int> pointIds;
        bool isLeaf = false;
        int  parent = -1;
    };

    vector<RSNode> nodes;
    int root = -1;
    int treeHeight = 0;
    vector<bool> reinsertedLevels;

    RStarTree(vector<vector<double>> &points, int dims);

    int findClosestIndexToTarget(const vector<double> &lo,
                                 const vector<double> &hi, int target);

private:
    int    newNode();
    int    buildSTR(vector<int> &ptIds, int depth);
    void   computeMBR(int nIdx);
    void   getAllEntries(int nIdx, vector<int> &entries);
    int    chooseSplitAxis(int nIdx);
    int    chooseSplitIndex(int nIdx, int axis);
    double computeOverlap(const vector<double> &lo1, const vector<double> &hi1,
                          const vector<double> &lo2, const vector<double> &hi2);
    double computeArea(const vector<double> &lo, const vector<double> &hi);
    bool   overlapsBox(const RSNode &node, const vector<double> &lo,
                       const vector<double> &hi);
    bool   pointInBox(int ptIdx, const vector<double> &lo,
                      const vector<double> &hi);
    double mindistToTarget(const RSNode &node, int target);
};

optional<Result> queryBestRT_rstar(Data &data, RStarTree &tree,
                                   int epsilon, double startVal, double endVal,
                                   double alpha, double beta);
