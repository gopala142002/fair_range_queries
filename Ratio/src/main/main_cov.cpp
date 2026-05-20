#include "../common/fair_range.h"

// --- Tree Selector ---
// 0 for Range Tree, 1 for R-Tree, 2 for X-Tree, 3 for R*-Tree, 4 for K-D Tree
#define TREE_TYPE 0

#if TREE_TYPE == 0
#include "../trees/range_tree_Synthetic.h"
#include "../query/range_tree_cov.h"
#elif TREE_TYPE == 1
#include "../trees/R_tree_Synthetic.h"
#include "../query/rtree_cov.h"
#elif TREE_TYPE == 2
#include "../trees/X_tree_Synthetic.h"
#include "../query/xtree_cov.h"
#elif TREE_TYPE == 3
#include "../trees/R_Star_Synthetic.h"
#include "../query/rstar_tree_cov.h"
#elif TREE_TYPE == 4
#include "../trees/KD_Tree_Synthetic.h"
#include "../query/kd_tree_cov.h"
#endif
#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <sstream>

int main(int argc, char *argv[]) {
  std::string filename = "data/synthetic_30k.csv";
  if (argc > 1) {
      filename = argv[1];
  }
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

  const int DIMS = 3;
  search_nodes_visited = 0;
  search_points_considered = 0;

  auto build_start = std::chrono::high_resolution_clock::now();
#if TREE_TYPE == 0
  std::cout << "\nBuilding 3D Layered Range Tree..." << std::endl;
  RangeTreeNode *root = build_tree(points_ptrs, 0, DIMS);
#elif TREE_TYPE == 1
  std::cout << "\nBuilding Custom R-Tree..." << std::endl;
  RTreeNode *root = build_rtree(points_ptrs);
#elif TREE_TYPE == 2
  std::cout << "\nBuilding Custom X-Tree..." << std::endl;
  XTreeNode *root = build_xtree(points_ptrs);
#elif TREE_TYPE == 3
  std::cout << "\nBuilding Custom R*-Tree..." << std::endl;
  RStarTreeNode *root = build_rstar_tree(points_ptrs);
#elif TREE_TYPE == 4
  std::cout << "\nBuilding Custom K-D Tree..." << std::endl;
  KDTreeNode *root = build_kdtree(points_ptrs);
#endif
  auto build_end = std::chrono::high_resolution_clock::now();
  auto build_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      build_end - build_start);
  std::cout << "Tree built in " << build_duration.count() << "ms." << std::endl;

  std::string line;
  while (true) {
    std::cout << "Enter epsilon start end alpha beta min_red min_blue (-1 to exit):\n";
    if (!std::getline(std::cin, line)) break;
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n")+1);
    if (line.empty()) continue;
    std::istringstream iss(line);
    double e, s, en, a, b, req_red, req_blue;
    if (!(iss >> e >> s >> en >> a >> b >> req_red >> req_blue)) continue;
    if (e == -1) break;
    
    if (e != epsilon) {
        epsilon = e;
        one_plus_epsilon = 1.0 + epsilon;
        points_data.clear();
        points_ptrs.clear();
        for (int i = 0; i < DATASET_SIZE; ++i) {
            double cr = cumulative_red[i] - cumulative_blue[i] * one_plus_epsilon;
            double cb = cumulative_blue[i] - cumulative_red[i] * one_plus_epsilon;
            points_data.emplace_back(cr, cb, i);
        }
        for (auto &p : points_data) points_ptrs.push_back(&p);
#if TREE_TYPE == 0
        delete_tree(root);
        root = build_tree(points_ptrs, 0, DIMS);
#elif TREE_TYPE == 1
        delete_rtree(root);
        root = build_rtree(points_ptrs);
#elif TREE_TYPE == 2
        delete_xtree(root);
        root = build_xtree(points_ptrs);
#elif TREE_TYPE == 3
        delete_rstar_tree(root);
        root = build_rstar_tree(points_ptrs);
#elif TREE_TYPE == 4
        delete_kdtree(root);
        root = build_kdtree(points_ptrs);
#endif
        std::cout << "Tree rebuilt for epsilon = " << epsilon << "\n";
    }

    std::cout << "Coverage: red>=" << req_red << " blue>=" << req_blue << "\n";
    
    auto q0 = std::chrono::high_resolution_clock::now();
#if TREE_TYPE == 0
    auto res = queryBestRT_cov(dataset, points_data, root, epsilon, s, en, a, b, req_red, req_blue, cumulative_red, cumulative_blue);
#elif TREE_TYPE == 1
    auto res = queryBestRTree_cov(dataset, points_data, root, epsilon, s, en, a, b, req_red, req_blue, cumulative_red, cumulative_blue);
#elif TREE_TYPE == 2
    auto res = queryBestXTree_cov(dataset, points_data, root, epsilon, s, en, a, b, req_red, req_blue, cumulative_red, cumulative_blue);
#elif TREE_TYPE == 3
    auto res = queryBestRStarTree_cov(dataset, points_data, root, epsilon, s, en, a, b, req_red, req_blue, cumulative_red, cumulative_blue);
#elif TREE_TYPE == 4
    auto res = queryBestKDTree_cov(dataset, points_data, root, epsilon, s, en, a, b, req_red, req_blue, cumulative_red, cumulative_blue);
#endif
    auto q1 = std::chrono::high_resolution_clock::now();

    if (!res) std::cout << "Best range = [NA,NA] (similarity = 0.0)\n";
    else std::cout << "Best range = [" << res->min_val << "," << res->max_val << "] (similarity = " << res->tversky << ")\n";
    std::cout << "Query executed in " << std::chrono::duration_cast<std::chrono::milliseconds>(q1-q0).count() << " ms\n";

    int query_i = -1;
    int query_j = -1;
    for (int i = 0; i < DATASET_SIZE; ++i) {
      if (dataset[i].value >= std::min(s, en) && dataset[i].value <= std::max(s, en)) {
        if (query_i == -1) query_i = i;
        query_j = i;
      }
    }
    if (query_i != -1) {
        find_best_fair_range_brute_force_cov(query_i, query_j, epsilon, a, b, req_red, req_blue, cumulative_red, cumulative_blue);
    }
  }

#if TREE_TYPE == 0
  delete_tree(root);
#elif TREE_TYPE == 1
  delete_rtree(root);
#elif TREE_TYPE == 2
  delete_xtree(root);
#elif TREE_TYPE == 3
  delete_rstar_tree(root);
#elif TREE_TYPE == 4
  delete_kdtree(root);
#endif
  return 0;
}
