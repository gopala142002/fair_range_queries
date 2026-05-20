#include "R_tree_Synthetic.h"
#include "../common/fair_range.h"
#include <algorithm>
#include <cmath>
#include <iostream>

const int MAX_ENTRIES = 16;

RTreeNode *build_rtree_recursive(std::vector<Point *> &pts, int start, int end,
                                 int axis) {
  if (start >= end)
    return nullptr;

  RTreeNode *node = new RTreeNode();

  // Compute bounding box
  node->min_cr = std::numeric_limits<double>::max();
  node->max_cr = -std::numeric_limits<double>::max();
  node->min_cb = std::numeric_limits<double>::max();
  node->max_cb = -std::numeric_limits<double>::max();

  for (int i = start; i < end; ++i) {
    node->min_cr = std::min(node->min_cr, pts[i]->coords[0]);
    node->max_cr = std::max(node->max_cr, pts[i]->coords[0]);
    node->min_cb = std::min(node->min_cb, pts[i]->coords[1]);
    node->max_cb = std::max(node->max_cb, pts[i]->coords[1]);
  }

  int total = end - start;

  if (total <= MAX_ENTRIES) {
    // Leaf node
    node->isLeaf = true;
    for (int i = start; i < end; ++i) {
      node->points.push_back(pts[i]);
    }
    return node;
  }

  // Internal node: sort by current axis and chunk
  std::sort(pts.begin() + start, pts.begin() + end, [axis](Point *a, Point *b) {
    return a->coords[axis] < b->coords[axis];
  });

  int groupSz =
      std::max(MAX_ENTRIES, (int)std::ceil((double)total / MAX_ENTRIES));

  for (int chunk_start = start; chunk_start < end; chunk_start += groupSz) {
    int chunk_end = std::min(chunk_start + groupSz, end);
    RTreeNode *child =
        build_rtree_recursive(pts, chunk_start, chunk_end, 1 - axis);
    if (child) {
      node->children.push_back(child);
    }
  }

  return node;
}

RTreeNode *build_rtree(std::vector<Point *> pts) {
  return build_rtree_recursive(pts, 0, pts.size(), 0);
}

void delete_rtree(RTreeNode *node) {
  if (!node)
    return;
  for (RTreeNode *child : node->children) {
    delete_rtree(child);
  }
  delete node;
}

void find_most_similar_fair_j_rtree(RTreeNode *node, const QueryBox &Q,
                                    int query_i, int query_j, int current_i,
                                    double &max_similarity, int &best_j) {
  if (!node)
    return;

  search_nodes_visited++;

  // Check intersection with QueryBox
  if (node->min_cr > Q.bounds[0].second || node->max_cr < Q.bounds[0].first)
    return;
  if (node->min_cb > Q.bounds[1].second || node->max_cb < Q.bounds[1].first)
    return;

  // If it's a leaf node, check the points
  if (node->isLeaf) {
    for (Point *point : node->points) {
      // Check if point is actually inside QueryBox
      if (point->coords[0] < Q.bounds[0].first ||
          point->coords[0] > Q.bounds[0].second ||
          point->coords[1] < Q.bounds[1].first ||
          point->coords[1] > Q.bounds[1].second) {
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

  // Traverse children
  for (RTreeNode *child : node->children) {
    find_most_similar_fair_j_rtree(child, Q, query_i, query_j, current_i,
                                   max_similarity, best_j);
  }
}
