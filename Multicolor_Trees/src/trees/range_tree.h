#pragma once
#include "../common/common.h"

// True multi-dimensional Range Tree (nested BST per dimension) with closest-index query.
struct RangeTree {
    int totalDims;
    vector<vector<double>> &P;

    struct RTNode {
        int dim, ptIdx;
        double splitVal;
        RTNode *left=nullptr, *right=nullptr, *nextTree=nullptr;
    };

    RTNode *root = nullptr;
    deque<RTNode> pool;

    RangeTree(vector<vector<double>> &points, int dims);
    int findClosestIndexToTarget(const vector<double> &lo, const vector<double> &hi, int target);

private:
    RTNode *newNode();
    RTNode *buildTree(vector<int> idx, int dim);
    void    queryClosest(RTNode *node, const vector<double> &lo, const vector<double> &hi,
                         int dim, int target, int &best, long long &bestDist);
    void    closestInIndexTree(RTNode *node, double lo, double hi,
                               int target, int &best, long long &bestDist);
    void    updateBest(int ptIdx, int target, int &best, long long &bestDist);
    RTNode *findSplitNode(RTNode *node, double lo, double hi);
    bool    pointInRange(int idx, const vector<double> &lo, const vector<double> &hi);
};

optional<Result> queryBestRT(Data &data, RangeTree &tree,
                             int epsilon, double startVal, double endVal,
                             double alpha, double beta);
