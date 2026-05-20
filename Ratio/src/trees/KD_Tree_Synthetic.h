#ifndef KD_TREE_SYNTHETIC_H
#define KD_TREE_SYNTHETIC_H

#include "range_tree_Synthetic.h" // Reuse Point, QueryBox, and calculate_tversky_index
#include <limits>
#include <vector>

struct KDTreeNode {
  double min_cr, max_cr;
  double min_cb, max_cb;
  bool isLeaf = false;
  int dim = 0;
  double split_val = 0.0;
  std::vector<Point *> points;
  KDTreeNode *left = nullptr;
  KDTreeNode *right = nullptr;
};

// Function to build the custom K-D Tree
KDTreeNode *build_kdtree(std::vector<Point *> pts);

// Function to delete the K-D Tree
void delete_kdtree(KDTreeNode *node);

// Function to query the K-D Tree
void find_most_similar_fair_j_kdtree(KDTreeNode *node, const QueryBox &Q,
                                     int query_i, int query_j, int current_i,
                                     double &max_similarity, int &best_j);

#endif
