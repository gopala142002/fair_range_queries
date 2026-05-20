#include "range_tree_cov.h"
#include <algorithm>
#include <iostream>
#include <limits>

int findCoveragePoint(int p, double req_red, double req_blue, 
                      const std::vector<double>& cum_red, const std::vector<double>& cum_blue) {
    int n = cum_red.size();
    double base_red = (p >= 0) ? cum_red[p] : 0.0;
    double base_blue = (p >= 0) ? cum_blue[p] : 0.0;
    
    auto it_red = std::lower_bound(cum_red.begin() + p + 1, cum_red.end(), base_red + req_red);
    if (it_red == cum_red.end()) return -1;
    
    auto it_blue = std::lower_bound(cum_blue.begin() + p + 1, cum_blue.end(), base_blue + req_blue);
    if (it_blue == cum_blue.end()) return -1;
    
    int R_red = std::distance(cum_red.begin(), it_red);
    int R_blue = std::distance(cum_blue.begin(), it_blue);
    
    return std::max(R_red, R_blue);
}

void find_most_similar_fair_j_cov(RangeTreeNode *node, const QueryBox &Q,
                                  int query_i, int query_j, int current_i,
                                  int level, int total_levels,
                                  int coveragePoint, double alpha, double beta,
                                  double &max_similarity, int &best_j) {
  if (!node)
    return;

  search_nodes_visited++;

  if (level == total_levels - 1) {
    search_points_considered++;
    if (inRangeQ(node->curr_point, Q, total_levels - 1)) {
      int j = node->curr_point->original_index;
      if (j >= current_i && j >= coveragePoint) {
        double sim = calculate_tversky_index(query_i, query_j, current_i, j, alpha, beta);
        if (sim > max_similarity) {
          max_similarity = sim;
          best_j = j;
        } else if (sim == max_similarity && sim >= 0.0) {
           int wBest = best_j - current_i, wCur = j - current_i;
           if (wCur < wBest || (wCur == wBest && j < best_j)) { best_j = j; }
        }
      }
    }

    int split_j = static_cast<int>(node->split_val);

    if (query_j < split_j) {
      find_most_similar_fair_j_cov(node->left, Q, query_i, query_j, current_i,
                               level, total_levels, coveragePoint, alpha, beta, max_similarity, best_j);

      int min_j_right = std::max(split_j + 1, coveragePoint);
      if (min_j_right >= coveragePoint) { 
          double max_possible_right_sim =
              calculate_tversky_index(query_i, query_j, current_i, min_j_right, alpha, beta);
          if (max_possible_right_sim > max_similarity) {
            find_most_similar_fair_j_cov(node->right, Q, query_i, query_j, current_i,
                                     level, total_levels, coveragePoint, alpha, beta, max_similarity, best_j);
          }
      }
    } else {
      find_most_similar_fair_j_cov(node->right, Q, query_i, query_j, current_i,
                               level, total_levels, coveragePoint, alpha, beta, max_similarity, best_j);

      int closest_j_left = split_j;
      if (closest_j_left >= coveragePoint) {
          double max_possible_left_sim =
              calculate_tversky_index(query_i, query_j, current_i, closest_j_left, alpha, beta);
          if (max_possible_left_sim > max_similarity) {
            find_most_similar_fair_j_cov(node->left, Q, query_i, query_j, current_i,
                                     level, total_levels, coveragePoint, alpha, beta, max_similarity, best_j);
          }
      }
    }
    return;
  }

  double high = Q.bounds[level].second;
  if (high < node->split_val) {
    find_most_similar_fair_j_cov(node->left, Q, query_i, query_j, current_i, level,
                             total_levels, coveragePoint, alpha, beta, max_similarity, best_j);
  } else {
    if (node->left && node->left->next_tree) {
      find_most_similar_fair_j_cov(node->left->next_tree, Q, query_i, query_j,
                               current_i, level + 1, total_levels, coveragePoint, alpha, beta,
                               max_similarity, best_j);
    }

    search_points_considered++;
    if (inRangeQ(node->curr_point, Q, total_levels - 1)) {
      int j = node->curr_point->original_index;
      if (j >= current_i && j >= coveragePoint) {
        double sim = calculate_tversky_index(query_i, query_j, current_i, j, alpha, beta);
        if (sim > max_similarity) {
          max_similarity = sim;
          best_j = j;
        } else if (sim == max_similarity && sim >= 0.0) {
           int wBest = best_j - current_i, wCur = j - current_i;
           if (wCur < wBest || (wCur == wBest && j < best_j)) { best_j = j; }
        }
      }
    }

    find_most_similar_fair_j_cov(node->right, Q, query_i, query_j, current_i, level,
                             total_levels, coveragePoint, alpha, beta, max_similarity, best_j);
  }
}

std::optional<OptResult>
queryBestRT_cov(const std::vector<DatasetPoint> &dataset,
                const std::vector<Point> &points_data, RangeTreeNode *tree_root,
                double epsilon, double startVal, double endVal, double alpha,
                double beta, double req_red, double req_blue,
                const std::vector<double>& cum_red, const std::vector<double>& cum_blue) {

  int n = dataset.size();
  if (n == 0) return std::nullopt;

  int query_i = -1;
  int query_j = -1;

  for (int i = 0; i < n; ++i) {
    if (dataset[i].value >= std::min(startVal, endVal) && dataset[i].value <= std::max(startVal, endVal)) {
      if (query_i == -1)
        query_i = i;
      query_j = i;
    }
  }

  if (query_i == -1) return std::nullopt;

  double bestT = -1.0;
  int best_i_tree = -1, best_j_tree = -1;
  int DIMS = 3;

  for (int i = 0; i < n; ++i) {
    int coveragePoint = findCoveragePoint(i - 1, req_red, req_blue, cum_red, cum_blue);
    if (coveragePoint == -1) continue;

    double max_cr = (i > 0) ? points_data[i - 1].coords[0] : 0.0;
    double max_cb = (i > 0) ? points_data[i - 1].coords[1] : 0.0;

    QueryBox Q;
    Q.bounds.push_back({-std::numeric_limits<double>::max(), max_cr});
    Q.bounds.push_back({-std::numeric_limits<double>::max(), max_cb});

    double current_max_sim = -1.0;
    int current_best_j = -1;

    find_most_similar_fair_j_cov(tree_root, Q, query_i, query_j, i, 0, DIMS, coveragePoint, alpha, beta,
                                 current_max_sim, current_best_j);

    if (current_max_sim > bestT) {
      bestT = current_max_sim;
      best_i_tree = i;
      best_j_tree = current_best_j;
    } else if (current_max_sim == bestT && bestT >= 0.0 && current_best_j != -1) {
       int wBest = best_j_tree - best_i_tree, wCur = current_best_j - i;
       if (wCur < wBest || (wCur == wBest && i < best_i_tree)) {
           best_i_tree = i;
           best_j_tree = current_best_j;
       }
    }
  }

  if (best_i_tree == -1) return std::nullopt;
  return OptResult{dataset[best_i_tree].value, dataset[best_j_tree].value, bestT};
}
