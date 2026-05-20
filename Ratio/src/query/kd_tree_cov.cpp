#include "kd_tree_cov.h"
#include <algorithm>
#include <iostream>
#include <limits>

void find_most_similar_fair_j_kdtree_cov(KDTreeNode *node, const QueryBox &Q,
                                     int query_i, int query_j, int current_i,
                                     int coveragePoint, double alpha, double beta,
                                     double &max_similarity, int &best_j) {
    if (!node) return;

    search_nodes_visited++;

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
        return;
    }

    find_most_similar_fair_j_kdtree_cov(node->left, Q, query_i, query_j, current_i, coveragePoint, alpha, beta, max_similarity, best_j);
    find_most_similar_fair_j_kdtree_cov(node->right, Q, query_i, query_j, current_i, coveragePoint, alpha, beta, max_similarity, best_j);
}

std::optional<OptResult>
queryBestKDTree_cov(const std::vector<DatasetPoint> &dataset,
                const std::vector<Point> &points_data, KDTreeNode *tree_root,
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

    find_most_similar_fair_j_kdtree_cov(tree_root, Q, query_i, query_j, i, coveragePoint, alpha, beta,
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
