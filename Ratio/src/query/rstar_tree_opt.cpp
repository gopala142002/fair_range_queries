#include "rstar_tree_opt.h"
#include <algorithm>
#include <functional>

// Global counters
extern long long search_nodes_visited;
extern long long search_points_considered;

// Helper bounds
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

static double ubLP1(int L, int qL, int qR, double b) {
  double Q = qR - qL + 1;
  return Q / (Q + b * (qL - L));
}
static double ubLP2(int L, int qL, int qR, double a) {
  double i = qR - L + 1;
  return i / (i + a * (L - qL));
}
static double ubRP1(int R, int qL, int qR, double b) {
  double Q = qR - qL + 1;
  return Q / (Q + b * (R - qR));
}
static double ubRP2(int R, int qL, int qR, double a) {
  double i = R - qL + 1;
  return i / (i + a * (qR - R));
}

std::optional<OptResult>
queryBestRT_rstar_opt(const std::vector<DatasetPoint> &dataset,
                      const std::vector<Point> &points_data,
                      RStarTreeNode *tree_root, double epsilon, double startVal,
                      double endVal, double alpha, double beta) {

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
  // Recursive search logic tailored for R*-Tree
  std::function<void(RStarTreeNode *, const QueryBox &)> searchLeftTree =
      [&](RStarTreeNode *node, const QueryBox &Q) {
        if (!node)
          return;
        search_nodes_visited++;

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
        for (RStarTreeNode *child : node->children) {
          searchLeftTree(child, Q);
        }
      };

  auto tryLeft = [&](int p) {
    current_p = p;
    QueryBox Q = getQ(p);
    searchLeftTree(tree_root, Q);
  };

  int current_r = -1;
  std::function<void(RStarTreeNode *, const QueryBox &)> searchRightTree =
      [&](RStarTreeNode *node, const QueryBox &Q) {
        if (!node)
          return;
        search_nodes_visited++;

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
              int candL = p_minus_1 + 2;
              if (candL <= current_r + 1) {
                updateResult(candL, current_r + 1, qL, qR, alpha, beta, bestT,
                             bestL, bestR);
              }
            }
          }
          return;
        }
        for (RStarTreeNode *child : node->children) {
          searchRightTree(child, Q);
        }
      };

  auto tryRight = [&](int r) {
    current_r = r;
    QueryBox Q;
    double min_cr = points_data[r].coords[0];
    double min_cb = points_data[r].coords[1];
    Q.bounds.push_back({min_cr, std::numeric_limits<double>::max()});
    Q.bounds.push_back({min_cb, std::numeric_limits<double>::max()});

    searchRightTree(tree_root, Q);

    QueryBox Q0 = getQ(0);
    if (points_data[r].coords[0] <= Q0.bounds[0].second &&
        points_data[r].coords[1] <= Q0.bounds[1].second) {
      updateResult(1, r + 1, qL, qR, alpha, beta, bestT, bestL, bestR);
    }
  };

  if (qR <= n - qL) {
    for (int p = qL - 2; p >= 0; --p) {
      int L = p + 1;
      if (ubLP1(L, qL, qR, beta) <= bestT)
        break;
      tryLeft(p);
    }
    for (int p = qL - 1; p <= qR - 1; ++p) {
      int L = p + 1;
      if (ubLP2(L, qL, qR, alpha) <= bestT)
        break;
      tryLeft(p);
    }
  } else {
    for (int r = qR - 1; r < n; ++r) {
      int R = r + 1;
      if (ubRP1(R, qL, qR, beta) <= bestT)
        break;
      tryRight(r);
    }
    for (int r = qR - 2; r >= qL - 1; --r) {
      int R = r + 1;
      if (ubRP2(R, qL, qR, alpha) <= bestT)
        break;
      tryRight(r);
    }
  }

  if (bestL == -1)
    return std::nullopt;
  return OptResult{dataset[bestL - 1].value, dataset[bestR - 1].value, bestT};
}