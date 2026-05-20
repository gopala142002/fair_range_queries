#include "KD_Tree_Synthetic.h"
#include "../common/fair_range.h"
#include <iostream>
#include <algorithm>
#include <cmath>

const int LEAF_CAP = 16;
const int totalDims = 2; // For Ratio project, it's always 2 (cr, cb)

void nthElement(std::vector<Point *> &a, int lo, int mid, int hi, int dim) {
    int l = lo, r = hi - 1;
    while (true) {
        int i = l, j = r;
        double pivot = a[(l + r) >> 1]->coords[dim];
        while (i <= j) {
            while (a[i]->coords[dim] < pivot) ++i;
            while (a[j]->coords[dim] > pivot) --j;
            if (i <= j) { std::swap(a[i], a[j]); ++i; --j; }
        }
        if (j < mid) l = i; else r = j;
        if (l >= mid && r <= mid) return;
    }
}

KDTreeNode *build_kdtree_recursive(std::vector<Point *> &idx, int loi, int hii, int depth) {
    if (loi >= hii) return nullptr;

    KDTreeNode *node = new KDTreeNode();
    node->min_cr = std::numeric_limits<double>::infinity();
    node->max_cr = -std::numeric_limits<double>::infinity();
    node->min_cb = std::numeric_limits<double>::infinity();
    node->max_cb = -std::numeric_limits<double>::infinity();

    for (int k = loi; k < hii; ++k) {
        Point *pt = idx[k];
        node->min_cr = std::min(node->min_cr, pt->coords[0]);
        node->max_cr = std::max(node->max_cr, pt->coords[0]);
        node->min_cb = std::min(node->min_cb, pt->coords[1]);
        node->max_cb = std::max(node->max_cb, pt->coords[1]);
    }

    int n = hii - loi;
    if (n <= LEAF_CAP) {
        node->isLeaf = true;
        node->points.assign(idx.begin() + loi, idx.begin() + hii);
        return node;
    }

    node->dim = depth % totalDims;
    int mid = loi + n / 2;
    nthElement(idx, loi, mid, hii, node->dim);
    node->split_val = idx[mid]->coords[node->dim];
    
    node->left = build_kdtree_recursive(idx, loi, mid, depth + 1);
    node->right = build_kdtree_recursive(idx, mid, hii, depth + 1);
    
    return node;
}

KDTreeNode *build_kdtree(std::vector<Point *> pts) {
    return build_kdtree_recursive(pts, 0, pts.size(), 0);
}

void delete_kdtree(KDTreeNode *node) {
    if (!node) return;
    delete_kdtree(node->left);
    delete_kdtree(node->right);
    delete node;
}

void find_most_similar_fair_j_kdtree(KDTreeNode *node, const QueryBox &Q,
                                     int query_i, int query_j, int current_i,
                                     double &max_similarity, int &best_j) {
    if (!node) return;

    search_nodes_visited++;

    // Check intersection with QueryBox
    if (node->min_cr > Q.bounds[0].second || node->max_cr < Q.bounds[0].first) return;
    if (node->min_cb > Q.bounds[1].second || node->max_cb < Q.bounds[1].first) return;

    if (node->isLeaf) {
        for (Point *point : node->points) {
            if (point->coords[0] < Q.bounds[0].first || point->coords[0] > Q.bounds[0].second ||
                point->coords[1] < Q.bounds[1].first || point->coords[1] > Q.bounds[1].second) {
                continue;
            }

            search_points_considered++;
            int j = point->original_index;
            if (j >= current_i) {
                double sim = calculate_tversky_index(query_i, query_j, current_i, j);
                if (sim > max_similarity) {
                    max_similarity = sim;
                    best_j = j;
                }
            }
        }
        return;
    }

    find_most_similar_fair_j_kdtree(node->left, Q, query_i, query_j, current_i, max_similarity, best_j);
    find_most_similar_fair_j_kdtree(node->right, Q, query_i, query_j, current_i, max_similarity, best_j);
}
