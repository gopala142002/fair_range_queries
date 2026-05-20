#include "range_tree_Synthetic.h"
#include "../common/fair_range.h"
#include <algorithm>

long long search_nodes_visited = 0;
long long search_points_considered = 0;

bool min_all(Point *p1, Point *p2) {
  if (!p1)
    return false;
  if (!p2)
    return true;
  for (size_t i = 0; i < p1->coords.size(); ++i) {
    if (p1->coords[i] < p2->coords[i])
      return true;
    if (p1->coords[i] > p2->coords[i])
      return false;
  }
  return false;
}

bool inRangeQ(Point *p, const QueryBox &Q, int dims_to_check) {
  for (int i = 0; i < dims_to_check; ++i) {
    if (p->coords[i] < Q.bounds[i].first || p->coords[i] > Q.bounds[i].second) {
      return false;
    }
  }
  return true;
}

void delete_tree(RangeTreeNode *node) {
  if (!node)
    return;
  delete_tree(node->left);
  delete_tree(node->right);
  delete_tree(node->next_tree);
  delete node;
}

RangeTreeNode *build_tree(std::vector<Point *> &pts, int level,
                          int total_levels) {
  if (pts.empty() || level >= total_levels)
    return nullptr;

  int dim = level;
  std::sort(pts.begin(), pts.end(), [&](Point *a, Point *b) {
    return a->coords[dim] < b->coords[dim];
  });

  int mid = pts.size() / 2;
  Point *curr = pts[mid];
  double split_val = curr->coords[dim];

  std::vector<Point *> left_pts(pts.begin(), pts.begin() + mid);
  std::vector<Point *> right_pts(pts.begin() + mid + 1, pts.end());

  RangeTreeNode *left = build_tree(left_pts, level, total_levels);
  RangeTreeNode *right = build_tree(right_pts, level, total_levels);
  RangeTreeNode *next = build_tree(pts, level + 1, total_levels);

  Point *min_p = curr;
  if (left && min_all(left->min_point, min_p))
    min_p = left->min_point;
  if (right && min_all(right->min_point, min_p))
    min_p = right->min_point;
  if (next && min_all(next->min_point, min_p))
    min_p = next->min_point;

  return new RangeTreeNode(level, split_val, (int)(pts.size()), left, right,
                           next, curr, min_p);
}

void find_most_similar_fair_j(RangeTreeNode *node, const QueryBox &Q,
                              int query_i, int query_j, int current_i,
                              int level, int total_levels,
                              double &max_similarity, int &best_j) {
  if (!node)
    return;

  search_nodes_visited++;

  if (level == total_levels - 1) {
    search_points_considered++;
    if (inRangeQ(node->curr_point, Q, total_levels - 1)) {
      int j = node->curr_point->original_index;
      if (j >= current_i) {
        double sim = calculate_tversky_index(query_i, query_j, current_i, j);
        if (sim > max_similarity) {
          max_similarity = sim;
          best_j = j;
        }
      }
    }

    int split_j = static_cast<int>(node->split_val);

    if (query_j < split_j) {
      find_most_similar_fair_j(node->left, Q, query_i, query_j, current_i,
                               level, total_levels, max_similarity, best_j);

      double max_possible_right_sim =
          calculate_tversky_index(query_i, query_j, current_i, split_j + 1);
      if (max_possible_right_sim > max_similarity) {
        find_most_similar_fair_j(node->right, Q, query_i, query_j, current_i,
                                 level, total_levels, max_similarity, best_j);
      }
    } else {
      find_most_similar_fair_j(node->right, Q, query_i, query_j, current_i,
                               level, total_levels, max_similarity, best_j);

      double max_possible_left_sim =
          calculate_tversky_index(query_i, query_j, current_i, split_j - 1);
      if (max_possible_left_sim > max_similarity) {
        find_most_similar_fair_j(node->left, Q, query_i, query_j, current_i,
                                 level, total_levels, max_similarity, best_j);
      }
    }
    return;
  }

  double high = Q.bounds[level].second;
  if (high < node->split_val) {
    find_most_similar_fair_j(node->left, Q, query_i, query_j, current_i, level,
                             total_levels, max_similarity, best_j);
  } else {
    if (node->left && node->left->next_tree) {
      find_most_similar_fair_j(node->left->next_tree, Q, query_i, query_j,
                               current_i, level + 1, total_levels,
                               max_similarity, best_j);
    }

    search_points_considered++;
    if (inRangeQ(node->curr_point, Q, total_levels - 1)) {
      int j = node->curr_point->original_index;
      if (j >= current_i) {
        double sim = calculate_tversky_index(query_i, query_j, current_i, j);
        if (sim > max_similarity) {
          max_similarity = sim;
          best_j = j;
        }
      }
    }

    find_most_similar_fair_j(node->right, Q, query_i, query_j, current_i, level,
                             total_levels, max_similarity, best_j);
  }
}
