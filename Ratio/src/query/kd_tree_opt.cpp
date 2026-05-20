#include "kd_tree_opt.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <vector>

extern long long search_nodes_visited;
extern long long search_points_considered;

// Binary search helper functions matching the data logic
static int lowerBoundKeys(const std::vector<DatasetPoint> &dataset,
                          double val) {
  auto it = std::lower_bound(
      dataset.begin(), dataset.end(), val,
      [](const DatasetPoint &dp, double v) { return dp.value < v; });
  return std::distance(dataset.begin(), it);
}

static int upperBoundKeys(const std::vector<DatasetPoint> &dataset,
                          double val) {
  auto it = std::upper_bound(
      dataset.begin(), dataset.end(), val,
      [](double v, const DatasetPoint &dp) { return v < dp.value; });
  return std::distance(dataset.begin(), it);
}

static void updateResult(int L, int R, int qL, int qR, double alpha,
                         double beta, double &bestT, int &bestL, int &bestR) {
  double t = calculate_tversky_index(L, R, qL, qR, alpha, beta);
  if (t > bestT) {
    bestT = t;
    bestL = L;
    bestR = R;
  } else if (t == bestT && t >= 0.0) {
    int wBest = bestR - bestL, wCur = R - L;
    if (wCur < wBest || (wCur == wBest && L < bestL) ||
        (wCur == wBest && L == bestL && R < bestR)) {
      bestL = L;
      bestR = R;
    }
  }
}

static double upperBoundLeftPhase1(int L, int qL, int qR, double beta) {
  double Q = qR - qL + 1;
  return Q / (Q + beta * (qL - L));
}

static double upperBoundLeftPhase2(int L, int qL, int qR, double alpha) {
  double inter = qR - L + 1;
  double onlyIn = L - qL;
  return inter / (inter + alpha * onlyIn);
}

static double upperBoundRightPhase1(int R, int qL, int qR, double beta) {
  double Q = qR - qL + 1;
  return Q / (Q + beta * (R - qR));
}

static double upperBoundRightPhase2(int R, int qL, int qR, double alpha) {
  double inter = R - qL + 1;
  double onlyIn = qR - R;
  return inter / (inter + alpha * onlyIn);
}

std::optional<OptResult>
queryBestKD_opt(const std::vector<DatasetPoint> &dataset,
                const std::vector<Point> &points_data, KDTreeNode *tree_root,
                double startVal, double endVal, double alpha, double beta) {

  int n = dataset.size();
  if (n == 0)
    return std::nullopt;

  int L0 = lowerBoundKeys(dataset, std::min(startVal, endVal));
  int R0 = upperBoundKeys(dataset, std::max(startVal, endVal)) - 1;
  L0 = std::max(L0, 0);
  R0 = std::min(R0, n - 1);
  if (L0 > R0)
    return std::nullopt;

  int qL = L0 + 1, qR = R0 + 1;

  double bestT = -1.0;
  int bestL = -1, bestR = -1;

  auto getQ = [&](int p) {
    QueryBox Q;
    double max_cr = (p > 0) ? points_data[p - 1].coords[0] : 0.0;
    double max_cb = (p > 0) ? points_data[p - 1].coords[1] : 0.0;
    Q.bounds.push_back({-std::numeric_limits<double>::max(), max_cr});
    Q.bounds.push_back({-std::numeric_limits<double>::max(), max_cb});
    return Q;
  };

  int current_p = -1;
  // Query tree to find all valid right boundaries (R) for a fixed left boundary
  // (L)
  std::function<void(KDTreeNode *, const QueryBox &)> searchLeftTree =
      [&](KDTreeNode *node, const QueryBox &Q) {
        if (!node)
          return;
        search_nodes_visited++;

        // Prune if node's MBR does not intersect Q
        if (node->min_cr > Q.bounds[0].second ||
            node->max_cr < Q.bounds[0].first)
          return;
        if (node->min_cb > Q.bounds[1].second ||
            node->max_cb < Q.bounds[1].first)
          return;

        if (node->isLeaf) {
          for (Point *point : node->points) {
            search_points_considered++;
            if (point->coords[0] >= Q.bounds[0].first &&
                point->coords[0] <= Q.bounds[0].second &&
                point->coords[1] >= Q.bounds[1].first &&
                point->coords[1] <= Q.bounds[1].second) {

              int r = point->original_index;
              int candR = r + 1;
              if (candR > current_p) {
                updateResult(current_p + 1, candR, qL, qR, alpha, beta, bestT,
                             bestL, bestR);
              }
            }
          }
          return;
        }
        searchLeftTree(node->left, Q);
        searchLeftTree(node->right, Q);
      };

  auto tryLeft = [&](int p) {
    current_p = p; // p is 0-indexed left boundary
    QueryBox Q = getQ(p);
    searchLeftTree(tree_root, Q);
  };

  int current_r = -1;
  // Query tree to find all valid left boundaries (L) for a fixed right boundary
  // (R)
  std::function<void(KDTreeNode *, const QueryBox &)> searchRightTree =
      [&](KDTreeNode *node, const QueryBox &Q) {
        if (!node)
          return;
        search_nodes_visited++;

        // Prune if node's MBR does not intersect Q
        if (node->min_cr > Q.bounds[0].second ||
            node->max_cr < Q.bounds[0].first)
          return;
        if (node->min_cb > Q.bounds[1].second ||
            node->max_cb < Q.bounds[1].first)
          return;

        if (node->isLeaf) {
          for (Point *point : node->points) {
            search_points_considered++;
            if (point->coords[0] >= Q.bounds[0].first &&
                point->coords[0] <= Q.bounds[0].second &&
                point->coords[1] >= Q.bounds[1].first &&
                point->coords[1] <= Q.bounds[1].second) {

              int p_minus_1 = point->original_index;
              int candL =
                  p_minus_1 + 2; // derived from p = p_minus_1 + 1, L = p + 1
              if (candL <= current_r + 1) {
                updateResult(candL, current_r + 1, qL, qR, alpha, beta, bestT,
                             bestL, bestR);
              }
            }
          }
          return;
        }
        searchRightTree(node->left, Q);
        searchRightTree(node->right, Q);
      };

  auto tryRight = [&](int r) {
    current_r = r; // r is 0-indexed right boundary
    QueryBox Q;
    double min_cr = points_data[r].coords[0];
    double min_cb = points_data[r].coords[1];
    Q.bounds.push_back({min_cr, std::numeric_limits<double>::max()});
    Q.bounds.push_back({min_cb, std::numeric_limits<double>::max()});

    searchRightTree(tree_root, Q);

    // Explicitly evaluate p=0 because points_data[-1] is not in the tree
    QueryBox Q0 = getQ(0);
    if (points_data[r].coords[0] <= Q0.bounds[0].second &&
        points_data[r].coords[1] <= Q0.bounds[1].second) {
      updateResult(1, r + 1, qL, qR, alpha, beta, bestT, bestL, bestR);
    }
  };

  if (qR <= n - qL) {
    // ---- LEFT SIDE smaller ----
    for (int p = qL - 2; p >= 0; --p) {
      int L = p + 1;
      if (upperBoundLeftPhase1(L, qL, qR, beta) <= bestT)
        break;
      tryLeft(p);
    }
    for (int p = qL - 1; p <= qR - 1; ++p) {
      int L = p + 1;
      if (upperBoundLeftPhase2(L, qL, qR, alpha) <= bestT)
        break;
      tryLeft(p);
    }
  } else {
    // ---- RIGHT SIDE smaller ----
    for (int r = qR - 1; r < n; ++r) {
      int R = r + 1;
      if (upperBoundRightPhase1(R, qL, qR, beta) <= bestT)
        break;
      tryRight(r);
    }
    for (int r = qR - 2; r >= qL - 1; --r) {
      int R = r + 1;
      if (upperBoundRightPhase2(R, qL, qR, alpha) <= bestT)
        break;
      tryRight(r);
    }
  }

  if (bestL == -1)
    return std::nullopt;
  return OptResult{dataset[bestL - 1].value, dataset[bestR - 1].value, bestT};
}