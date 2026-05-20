#include "range_tree_opt.h"
#include <algorithm>
#include <functional>
#include <limits>

// Global counters (ensure these are declared in your main or common files)
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

// Custom fractional-cascading traverser specifically for the RangeTreeNode
// format
static void searchRangeTree(RangeTreeNode *node, const QueryBox &Q, int level,
                            int total_levels,
                            const std::function<void(Point *)> &callback) {
  if (!node)
    return;
  search_nodes_visited++;

  double low = Q.bounds[level].first;
  double high = Q.bounds[level].second;

  // Base case: last dimension
  if (level == total_levels - 1) {
    search_points_considered++;
    if (node->curr_point) {
      bool ok = true;
      for (int i = 0; i < total_levels; ++i) {
        if (node->curr_point->coords[i] < Q.bounds[i].first ||
            node->curr_point->coords[i] > Q.bounds[i].second) {
          ok = false;
          break;
        }
      }
      if (ok)
        callback(node->curr_point);
    }
    if (low < node->split_val)
      searchRangeTree(node->left, Q, level, total_levels, callback);
    if (high >= node->split_val)
      searchRangeTree(node->right, Q, level, total_levels, callback);
    return;
  }

  // Fractional cascading: tryLeft bound logic ([-inf, high])
  if (low == -std::numeric_limits<double>::max()) {
    if (high < node->split_val) {
      searchRangeTree(node->left, Q, level, total_levels, callback);
    } else {
      // High bounds split_val, so left tree is fully contained in [-inf, high].
      if (node->left) {
        if (node->left->next_tree) {
          searchRangeTree(node->left->next_tree, Q, level + 1, total_levels,
                          callback);
        } else {
          searchRangeTree(node->left, Q, level, total_levels, callback);
        }
      }

      search_points_considered++;
      if (node->curr_point) {
        bool ok = true;
        for (int i = 0; i < total_levels; ++i) {
          if (node->curr_point->coords[i] < Q.bounds[i].first ||
              node->curr_point->coords[i] > Q.bounds[i].second) {
            ok = false;
            break;
          }
        }
        if (ok)
          callback(node->curr_point);
      }
      // Must still verify the right path independently
      searchRangeTree(node->right, Q, level, total_levels, callback);
    }
  }
  // Fractional cascading: tryRight bound logic ([low, +inf])
  else if (high == std::numeric_limits<double>::max()) {
    if (low >= node->split_val) {
      searchRangeTree(node->right, Q, level, total_levels, callback);
    } else {
      // Low is below split_val, so right tree is fully contained in [low,
      // +inf].
      if (node->right) {
        if (node->right->next_tree) {
          searchRangeTree(node->right->next_tree, Q, level + 1, total_levels,
                          callback);
        } else {
          searchRangeTree(node->right, Q, level, total_levels, callback);
        }
      }

      search_points_considered++;
      if (node->curr_point) {
        bool ok = true;
        for (int i = 0; i < total_levels; ++i) {
          if (node->curr_point->coords[i] < Q.bounds[i].first ||
              node->curr_point->coords[i] > Q.bounds[i].second) {
            ok = false;
            break;
          }
        }
        if (ok)
          callback(node->curr_point);
      }
      // Must still verify the left path independently
      searchRangeTree(node->left, Q, level, total_levels, callback);
    }
  }
}

std::optional<OptResult>
queryBestRT_opt(const std::vector<DatasetPoint> &dataset,
                const std::vector<Point> &points_data, RangeTreeNode *tree_root,
                double epsilon, double startVal, double endVal, double alpha,
                double beta) {

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
  int total_levels = 2; // For your Ratio 2D dataset context

  auto tryLeft = [&](int p) {
    current_p = p;
    QueryBox Q = getQ(p);
    searchRangeTree(tree_root, Q, 0, total_levels, [&](Point *point) {
      int r = point->original_index;
      int candR = r + 1;
      if (candR > current_p) {
        updateResult(current_p + 1, candR, qL, qR, alpha, beta, bestT, bestL,
                     bestR);
      }
    });
  };

  int current_r = -1;
  auto tryRight = [&](int r) {
    current_r = r;
    QueryBox Q;
    double min_cr = points_data[r].coords[0];
    double min_cb = points_data[r].coords[1];
    Q.bounds.push_back({min_cr, std::numeric_limits<double>::max()});
    Q.bounds.push_back({min_cb, std::numeric_limits<double>::max()});

    searchRangeTree(tree_root, Q, 0, total_levels, [&](Point *point) {
      int p_minus_1 = point->original_index;
      int candL = p_minus_1 + 2;
      if (candL <= current_r + 1) {
        updateResult(candL, current_r + 1, qL, qR, alpha, beta, bestT, bestL,
                     bestR);
      }
    });

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