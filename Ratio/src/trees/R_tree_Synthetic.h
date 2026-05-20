#ifndef R_TREE_SYNTHETIC_H
#define R_TREE_SYNTHETIC_H

#include "range_tree_Synthetic.h" // Reuse Point, QueryBox, and calculate_tversky_index
#include <limits>
#include <vector>

struct RTreeNode {
  double min_cr, max_cr;
  double min_cb, max_cb;
  bool isLeaf = false;
  std::vector<Point *> points;
  std::vector<RTreeNode *> children;
};

// Function to build the custom binary R-Tree
RTreeNode *build_rtree(std::vector<Point *> pts);

// Function to delete the R-Tree
void delete_rtree(RTreeNode *node);

// Function to query the R-Tree
void find_most_similar_fair_j_rtree(RTreeNode *node, const QueryBox &Q,
                                    int query_i, int query_j, int current_i,
                                    double &max_similarity, int &best_j);

#endif
