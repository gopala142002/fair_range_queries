#include "../common/fair_range.h"
#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// --- Tree Selector ---
// 0 for Range Tree, 1 for R-Tree, 2 for X-Tree, 3 for R*-Tree, 4 for K-D Tree
#define TREE_TYPE 4

#if TREE_TYPE == 0
#include "../trees/range_tree_Synthetic.h"
#elif TREE_TYPE == 1
#include "../trees/R_tree_Synthetic.h"
#elif TREE_TYPE == 2
#include "../trees/X_tree_Synthetic.h"
#elif TREE_TYPE == 3
#include "../trees/R_Star_Synthetic.h"
#elif TREE_TYPE == 4
#include "../trees/KD_Tree_Synthetic.h"
#endif

// Define the extern globals here in exactly ONE place
int main() {
  [[maybe_unused]] const int DIMS = 3;

  // Fixed path relative to the root Make execution directory
  std::string filename = "data/synthetic_30k.csv";
  std::vector<DatasetPoint> dataset = read_custom_dataset(filename);

  if (dataset.empty()) {
    std::cerr << "Dataset could not be loaded or is empty. Exiting."
              << std::endl;
    return 1;
  }
  const int DATASET_SIZE = dataset.size();
  std::cout << "Successfully loaded and sorted " << DATASET_SIZE
            << " records from " << filename << std::endl;

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

  double epsilon = 10;
  std::cout << "--- Fairness Range Optimizer ---" << std::endl;

  double start_val = 1.0;
  double end_val = 10000.0;

  std::cout << "\nSearching for query range corresponding to values between "
            << start_val << " and " << end_val << std::endl;
  int query_i = -1;
  int query_j = -1;

  for (int i = 0; i < DATASET_SIZE; ++i) {
    if (dataset[i].value >= start_val && dataset[i].value <= end_val) {
      if (query_i == -1)
        query_i = i;
      query_j = i;
    }
  }

  if (query_i == -1) {
    std::cerr << "Error: No data points found in the specified range. Exiting."
              << std::endl;
    return 1;
  }

  std::cout << "\nInitial Query Range (by index) is [" << query_i << ", "
            << query_j << "]" << std::endl;
  std::cout << "Initial Query Range (by values) is [" << dataset[query_i].value
            << ", " << dataset[query_j].value << "]" << std::endl;

  // Assuming `Point` and `QueryBox` are defined in your tree headers
  std::vector<Point> points_data;
  points_data.reserve(DATASET_SIZE);

  double one_plus_epsilon = 1.0 + epsilon;
  for (int i = 0; i < DATASET_SIZE; ++i) {
    double cr = cumulative_red[i] - cumulative_blue[i] * one_plus_epsilon;
    double cb = cumulative_blue[i] - cumulative_red[i] * one_plus_epsilon;
    points_data.emplace_back(cr, cb, i);
  }

  std::vector<Point *> points_ptrs;
  points_ptrs.reserve(DATASET_SIZE);
  for (auto &p : points_data) {
    points_ptrs.push_back(&p);
  }

  search_nodes_visited = 0;
  search_points_considered = 0;

  auto build_start = std::chrono::high_resolution_clock::now();
#if TREE_TYPE == 0
  std::cout << "\nBuilding 3D Layered Range Tree..." << std::endl;
  RangeTreeNode *root = build_tree(points_ptrs, 0, DIMS);
#elif TREE_TYPE == 1
  std::cout << "\nBuilding Custom R-Tree..." << std::endl;
  RTreeNode *root_rtree = build_rtree(points_ptrs);
#elif TREE_TYPE == 2
  std::cout << "\nBuilding Custom X-Tree..." << std::endl;
  XTreeNode *root_xtree = build_xtree(points_ptrs);
#elif TREE_TYPE == 3
  std::cout << "\nBuilding Custom R*-Tree..." << std::endl;
  RStarTreeNode *root_rstar = build_rstar_tree(points_ptrs);
#elif TREE_TYPE == 4
  std::cout << "\nBuilding Custom K-D Tree..." << std::endl;
  KDTreeNode *root_kdtree = build_kdtree(points_ptrs);
#endif
  auto build_end = std::chrono::high_resolution_clock::now();
  auto build_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      build_end - build_start);
  std::cout << "Tree built in " << build_duration.count() << "ms." << std::endl;

  auto tree_query_start = std::chrono::high_resolution_clock::now();

  int best_i_tree = -1;
  int best_j_tree = -1;
  double max_similarity_tree = -1.0;

  for (int i = 0; i < DATASET_SIZE; ++i) {
    double max_cr = (i > 0) ? points_data[i - 1].coords[0] : 0.0;
    double max_cb = (i > 0) ? points_data[i - 1].coords[1] : 0.0;

    QueryBox Q;
    Q.bounds.push_back({-std::numeric_limits<double>::max(), max_cr});
    Q.bounds.push_back({-std::numeric_limits<double>::max(), max_cb});

    double current_max_sim = -1.0;
    int current_best_j = -1;

#if TREE_TYPE == 0
    find_most_similar_fair_j(root, Q, query_i, query_j, i, 0, DIMS,
                             current_max_sim, current_best_j);
#elif TREE_TYPE == 1
    find_most_similar_fair_j_rtree(root_rtree, Q, query_i, query_j, i,
                                   current_max_sim, current_best_j);
#elif TREE_TYPE == 2
    find_most_similar_fair_j_xtree(root_xtree, Q, query_i, query_j, i,
                                   current_max_sim, current_best_j);
#elif TREE_TYPE == 3
    find_most_similar_fair_j_rstar_tree(root_rstar, Q, query_i, query_j, i,
                                        current_max_sim, current_best_j);
#elif TREE_TYPE == 4
    find_most_similar_fair_j_kdtree(root_kdtree, Q, query_i, query_j, i,
                                    current_max_sim, current_best_j);
#endif

    if (current_max_sim > max_similarity_tree) {
      max_similarity_tree = current_max_sim;
      best_i_tree = i;
      best_j_tree = current_best_j;
    }
  }
  auto tree_query_end = std::chrono::high_resolution_clock::now();
  auto tree_query_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(tree_query_end -
                                                            tree_query_start);

#if TREE_TYPE == 0
  std::cout << "\n--- 3D Range-Tree Results ---" << std::endl;
#elif TREE_TYPE == 1
  std::cout << "\n--- R-Tree Results ---" << std::endl;
#elif TREE_TYPE == 2
  std::cout << "\n--- X-Tree Results ---" << std::endl;
#elif TREE_TYPE == 3
  std::cout << "\n--- R*-Tree Results ---" << std::endl;
#elif TREE_TYPE == 4
  std::cout << "\n--- K-D Tree Results ---" << std::endl;
#endif
  if (best_j_tree != -1) {
    std::cout << "Best Fair Range Index Found: [" << best_i_tree << ", "
              << best_j_tree << "]" << std::endl;
    std::cout << "Tversky Index: " << max_similarity_tree << std::endl;
    std::cout << "Corresponding values: [" << dataset[best_i_tree].value << ", "
              << dataset[best_j_tree].value << "]" << std::endl;
  } else {
    std::cout << "No fair range could be found." << std::endl;
  }
  std::cout << "Optimized 3D search total took: " << tree_query_duration.count()
            << " ms" << std::endl;
  std::cout << "  -> Total nodes visited: " << search_nodes_visited
            << std::endl;
  std::cout << "  -> Points checked inRangeQ: " << search_points_considered
            << std::endl;

#if TREE_TYPE == 0
  delete_tree(root);
  root = nullptr;
#elif TREE_TYPE == 1
  delete_rtree(root_rtree);
  root_rtree = nullptr;
#elif TREE_TYPE == 2
  delete_xtree(root_xtree);
  root_xtree = nullptr;
#elif TREE_TYPE == 3
  delete_rstar_tree(root_rstar);
  root_rstar = nullptr;
#elif TREE_TYPE == 4
  delete_kdtree(root_kdtree);
  root_kdtree = nullptr;
#endif

  auto brute_start = std::chrono::high_resolution_clock::now();
  find_best_fair_range_brute_force(query_i, query_j, epsilon, cumulative_red,
                                   cumulative_blue);
  auto brute_end = std::chrono::high_resolution_clock::now();
  auto brute_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      brute_end - brute_start);
  std::cout << "Brute-force search took: " << brute_duration.count() << " ms"
            << std::endl;

  return 0;
}