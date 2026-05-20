#ifndef X_TREE_SYNTHETIC_H
#define X_TREE_SYNTHETIC_H

#include "range_tree_Synthetic.h" // Reuse Point, QueryBox, and calculate_tversky_index
#include <limits>
#include <vector>

struct XTreeNode {
  double min_cr, max_cr;
  double min_cb, max_cb;
  bool isLeaf = false;
  bool isSupernode = false;
  std::vector<Point *> points;
  std::vector<XTreeNode *> children;
};

// Function to build the custom X-Tree
// MAX_OVERLAP defines the threshold of bounding box overlap allowed. If it exceeds this, we form a supernode.
// MAX_POINTS defines the minimum points to continue splitting (we can't split if there are fewer points).
XTreeNode *build_xtree(std::vector<Point *> pts, double max_overlap = 0.2, int max_points_leaf = 10);

// Function to delete the X-Tree
void delete_xtree(XTreeNode *node);

// Function to query the X-Tree
void find_most_similar_fair_j_xtree(XTreeNode *node, const QueryBox &Q,
                                    int query_i, int query_j, int current_i,
                                    double &max_similarity, int &best_j);

#endif
