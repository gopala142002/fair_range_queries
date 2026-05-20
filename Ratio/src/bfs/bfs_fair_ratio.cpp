#include "../common/fair_range.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <queue>
#include <unordered_set>

// Local helper structures and functions are kept internal to this file
struct BFSNode {
  int L;
  int R;
  double sim;

  bool operator<(const BFSNode &other) const {
    if (sim != other.sim)
      return sim < other.sim;

    int w_this = R - L;
    int w_other = other.R - other.L;
    if (w_this != w_other)
      return w_this > w_other;

    if (L != other.L)
      return L > other.L;
    return R > other.R;
  }
};

int next_right_distinct_left(const std::vector<DatasetPoint> &s, int L, int R) {
  if (L >= R)
    return -1;
  double val = s[L].value;
  int i = L + 1;
  while (i <= R && s[i].value == val)
    i++;
  return (i <= R) ? i : -1;
}

int prev_left_distinct_left(const std::vector<DatasetPoint> &s, int L) {
  if (L <= 0)
    return -1;
  double val = s[L].value;
  int i = L - 1;
  while (i >= 0 && s[i].value == val)
    i--;
  return (i >= 0) ? i : -1;
}

int next_right_distinct_right(const std::vector<DatasetPoint> &s, int L,
                              int R) {
  if (L >= R)
    return -1;
  double val = s[R].value;
  int i = R - 1;
  while (i >= L && s[i].value == val)
    i--;
  return (i >= L) ? i : -1;
}

int prev_left_distinct_right(const std::vector<DatasetPoint> &s, int R) {
  int n = s.size();
  if (R >= n - 1)
    return -1;
  double val = s[R].value;
  int i = R + 1;
  while (i < n && s[i].value == val)
    i++;
  return (i < n) ? i : -1;
}

inline uint64_t pack_key(int L, int R) {
  return (static_cast<uint64_t>(L) << 32) | static_cast<uint32_t>(R);
}

// Main algorithmic implementation
void find_best_fair_range_bfs(int query_i, int query_j, double epsilon,
                              const std::vector<double> &cumulative_red,
                              const std::vector<double> &cumulative_blue,
                              const std::vector<DatasetPoint> &dataset) {

  double lower_bound = 1.0 / (1.0 + epsilon);
  double upper_bound = 1.0 + epsilon;

  std::priority_queue<BFSNode> max_heap;
  std::unordered_set<uint64_t> visited;

  max_heap.push({query_i, query_j, 1.0});
  visited.insert(pack_key(query_i, query_j));

  int best_i = -1;
  int best_j = -1;
  double max_similarity = -1.0;
  long long nodes_explored = 0;

  while (!max_heap.empty()) {
    BFSNode current = max_heap.top();
    max_heap.pop();
    nodes_explored++;

    int L = current.L;
    int R = current.R;

    double red_score =
        cumulative_red[R] - (L > 0 ? cumulative_red[L - 1] : 0.0);
    double blue_score =
        cumulative_blue[R] - (L > 0 ? cumulative_blue[L - 1] : 0.0);

    bool is_fair = false;
    if (blue_score == 0.0) {
      if (red_score == 0.0)
        is_fair = true;
    } else {
      double ratio = red_score / blue_score;
      if (ratio >= lower_bound && ratio <= upper_bound) {
        is_fair = true;
      }
    }

    if (is_fair) {
      best_i = L;
      best_j = R;
      max_similarity = current.sim;
      break;
    }

    std::vector<std::pair<int, int>> neighbors;

    int nL = next_right_distinct_left(dataset, L, R);
    if (nL != -1)
      neighbors.push_back({nL, R});

    int pL = prev_left_distinct_left(dataset, L);
    if (pL != -1)
      neighbors.push_back({pL, R});

    int nR = next_right_distinct_right(dataset, L, R);
    if (nR != -1)
      neighbors.push_back({L, nR});

    int pR = prev_left_distinct_right(dataset, R);
    if (pR != -1)
      neighbors.push_back({L, pR});

    for (const auto &neighbor : neighbors) {
      int nb_L = neighbor.first;
      int nb_R = neighbor.second;
      uint64_t key = pack_key(nb_L, nb_R);

      if (visited.find(key) == visited.end()) {
        visited.insert(key);
        double sim = calculate_tversky_index(query_i, query_j, nb_L, nb_R);
        max_heap.push({nb_L, nb_R, sim});
      }
    }
  }

  std::cout << "\n--- Best First Search (BFS) Results ---" << std::endl;
  if (best_j != -1) {
    std::cout << "Best Fair Range Found Index: [" << best_i << ", " << best_j
              << "]" << std::endl;
    std::cout << "Tversky Index: " << max_similarity << std::endl;
    std::cout << "Total ranges explored by BFS: " << nodes_explored
              << std::endl;
  } else {
    std::cout << "No fair range could be found using the BFS method."
              << std::endl;
    std::cout << "Total ranges explored by BFS: " << nodes_explored
              << std::endl;
  }
}