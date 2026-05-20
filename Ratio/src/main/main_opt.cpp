#include "../common/fair_range.h"
#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// --- Tree Selector ---
// 0 for Range Tree, 1 for R-Tree, 2 for X-Tree, 3 for R*-Tree, 4 for K-D Tree
#define TREE_TYPE 1

#if TREE_TYPE == 0
#include "../query/range_tree_opt.h"
#include "../trees/range_tree_Synthetic.h"
#elif TREE_TYPE == 1
#include "../query/rtree_opt.h"
#include "../trees/R_tree_Synthetic.h"
#elif TREE_TYPE == 2
#include "../query/x_tree_opt.h"
#include "../trees/X_tree_Synthetic.h"
#elif TREE_TYPE == 3
#include "../query/rstar_tree_opt.h"
#include "../trees/R_Star_Synthetic.h"
#elif TREE_TYPE == 4
#include "../query/kd_tree_opt.h"
#include "../trees/KD_Tree_Synthetic.h"
#endif

// Global counters for metrics tracking
extern long long search_nodes_visited;
extern long long search_points_considered;

int main() {
  std::string file_path = "data/synthetic_30k.csv";

  // 1. Load dataset
  std::vector<DatasetPoint> dataset = read_custom_dataset(file_path);
  if (dataset.empty()) {
    return 1;
  }
  const int DATASET_SIZE = dataset.size();
  std::cout << "Successfully loaded and sorted " << DATASET_SIZE
            << " records from " << file_path << std::endl;

  // 2. Prepare pointers and points for tree building
  const double WEIGHT_RED = 2.0;
  const double WEIGHT_BLUE = 1.0;

  std::vector<double> cumulative_red(DATASET_SIZE, 0.0);
  std::vector<double> cumulative_blue(DATASET_SIZE, 0.0);

  for (int i = 0; i < DATASET_SIZE; ++i) {
    double prev_red = (i > 0) ? cumulative_red[i - 1] : 0.0;
    double prev_blue = (i > 0) ? cumulative_blue[i - 1] : 0.0;

    if (dataset[i].color == "red") {
      cumulative_red[i] = prev_red + WEIGHT_RED;
      cumulative_blue[i] = prev_blue;
    } else {
      cumulative_red[i] = prev_red;
      cumulative_blue[i] = prev_blue + WEIGHT_BLUE;
    }
  }

  double epsilon = 1;
  double alpha = 0.5;
  double beta = 0.5;

  std::cout << "--- Fairness Range Optimizer ---" << std::endl;
  std::cout << "Using Tree Type: " << TREE_TYPE << std::endl;

  double start_val = 1.0;
  double end_val = 10000.0;
  std::cout << "\nSearching for query range corresponding to values between "
            << start_val << " and " << end_val << std::endl;

  int query_i = -1, query_j = -1;
  for (int i = 0; i < DATASET_SIZE; ++i) {
    if (dataset[i].value >= start_val && dataset[i].value <= end_val) {
      if (query_i == -1)
        query_i = i;
      query_j = i;
    }
  }

  std::cout << "Initial Query Range (by index) is [" << query_i << ", "
            << query_j << "]" << std::endl;
  std::cout << "Initial Query Range (by values) is [" << dataset[query_i].value
            << ", " << dataset[query_j].value << "]" << std::endl;

  std::vector<Point> points_data;
  points_data.reserve(DATASET_SIZE);

  double one_plus_epsilon = 1.0 + epsilon;
  for (int i = 0; i < DATASET_SIZE; ++i) {
    double cr = cumulative_red[i] - cumulative_blue[i] * one_plus_epsilon;
    double cb = cumulative_blue[i] - cumulative_red[i] * one_plus_epsilon;
    points_data.emplace_back(cr, cb, i);
  }

  // Set up points_ptrs which is required for building BOTH K-D Tree and R*-Tree
  std::vector<Point *> points_ptrs;
  points_ptrs.reserve(DATASET_SIZE);
  for (auto &p : points_data) {
    points_ptrs.push_back(&p);
  }

  // Build the appropriate tree based on TREE_TYPE
  auto build_start = std::chrono::high_resolution_clock::now();

#if TREE_TYPE == 0
  std::cout << "\nBuilding Range Tree for Optimization..." << std::endl;
  RangeTreeNode *tree_root = build_tree(points_ptrs, 0, 2);
#elif TREE_TYPE == 1
  std::cout << "\nBuilding R-Tree for Optimization..." << std::endl;
  RTreeNode *tree_root = build_rtree(points_ptrs);
#elif TREE_TYPE == 2
  std::cout << "\nBuilding X-Tree for Optimization..." << std::endl;
  XTreeNode *tree_root = build_xtree(points_ptrs, 0.2, 16);
#elif TREE_TYPE == 3
  std::cout << "\nBuilding R*-Tree for Optimization..." << std::endl;
  RStarTreeNode *tree_root = build_rstar_tree(points_ptrs);
#elif TREE_TYPE == 4
  std::cout << "\nBuilding Custom K-D Tree for Optimization..." << std::endl;
  KDTreeNode *tree_root = build_kdtree(points_ptrs);
#endif

  auto build_end = std::chrono::high_resolution_clock::now();
  auto build_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            build_end - build_start)
                            .count();
  std::cout << "Tree built in " << build_duration << "ms.\n" << std::endl;

  // 3. Search for the optimal range
  search_nodes_visited = 0;
  search_points_considered = 0;

  auto search_start = std::chrono::high_resolution_clock::now();
  std::optional<OptResult> result = std::nullopt;

#if TREE_TYPE == 0
  result = queryBestRT_opt(dataset, points_data, tree_root, epsilon, start_val,
                           end_val, alpha, beta);
#elif TREE_TYPE == 1
  result = queryBestRT_rtree_opt(dataset, points_data, tree_root, epsilon,
                                 start_val, end_val, alpha, beta);
#elif TREE_TYPE == 2
  result = queryBestXT_opt(dataset, points_data, tree_root, epsilon, start_val,
                           end_val, alpha, beta);
#elif TREE_TYPE == 3
  result = queryBestRT_rstar_opt(dataset, points_data, tree_root, epsilon,
                                 start_val, end_val, alpha, beta);
#elif TREE_TYPE == 4
  result = queryBestKD_opt(dataset, points_data, tree_root, start_val, end_val,
                           alpha, beta);
#endif

  auto search_end = std::chrono::high_resolution_clock::now();
  auto search_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                             search_end - search_start)
                             .count();

  // 4. Print Tree Results
  std::cout << "--- Optimized Tree Results ---" << std::endl;
  if (result.has_value()) {
    int best_i_tree = -1, best_j_tree = -1;
    for (size_t i = 0; i < dataset.size(); ++i) {
      if (dataset[i].value == result->min_val)
        best_i_tree = i;
      if (dataset[i].value == result->max_val)
        best_j_tree = i;
    }
    std::cout << "Best Fair Range Index Found: [" << best_i_tree << ", "
              << best_j_tree << "]" << std::endl;
    std::cout << "Tversky Index: " << result->tversky << std::endl;
    std::cout << "Corresponding values: [" << result->min_val << ", "
              << result->max_val << "]" << std::endl;
  } else {
    std::cout << "No fair range found." << std::endl;
  }

  std::cout << "Optimized search total took: " << search_duration << " ms"
            << std::endl;
  std::cout << "  -> Total nodes visited (external): " << search_nodes_visited
            << std::endl;

  // Cleanup dynamically allocated tree

#if TREE_TYPE == 0
  delete_tree(tree_root);
#elif TREE_TYPE == 1
  delete_rtree(tree_root);
#elif TREE_TYPE == 2
  delete_xtree(tree_root);
#elif TREE_TYPE == 3
  delete_rstar_tree(tree_root);
#elif TREE_TYPE == 4
  delete_kdtree(tree_root);
#endif

  // 5. Brute Force Validation
  auto brute_start = std::chrono::high_resolution_clock::now();
  find_best_fair_range_brute_force(query_i, query_j, epsilon, cumulative_red,
                                   cumulative_blue);
  auto brute_end = std::chrono::high_resolution_clock::now();
  auto brute_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            brute_end - brute_start)
                            .count();
  std::cout << "Brute-force search took: " << brute_duration << " ms\n"
            << std::endl;

  return 0;
}