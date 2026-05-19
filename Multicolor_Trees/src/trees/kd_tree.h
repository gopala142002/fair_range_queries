#pragma once
#include "../common/common.h"

// KD-tree for multi-dimensional point existence queries.
struct KDTree {
    static constexpr int LEAF_CAP = 32;
    int dims;
    vector<vector<double>> P;

    struct Node {
        vector<double> lo, hi;
        int dim; double split;
        unique_ptr<Node> left, right;
        vector<int> idxs;
        bool isLeaf = false;
    };

    unique_ptr<Node> root;

    KDTree(const vector<vector<double>> &points, int dims);
    bool pointExistsInRange(const vector<double> &lo, const vector<double> &hi) const;

private:
    void nthElement(vector<int> &a, int lo, int mid, int hi, int dim);
    unique_ptr<Node> build(vector<int> &idx, int loi, int hii, int depth);
    bool overlaps(const Node *node, const vector<double> &qlo, const vector<double> &qhi) const;
    bool contains(const vector<double> &pt,  const vector<double> &qlo, const vector<double> &qhi) const;
    bool search(const Node *node, const vector<double> &lo, const vector<double> &hi) const;
};

optional<Result> queryBestKD(const Data &data, const KDTree &tree,
                             int epsilon, double startVal, double endVal,
                             double alpha, double beta);
