#include "../common/fair_range.h"
#include "../trees/range_tree_Synthetic.h"
#include "range_tree_cov.h"
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
  std::cout << "\nBuilding 3D Layered Range Tree..." << std::endl;
  RangeTreeNode *root = build_tree(points_ptrs, 0, DIMS);
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
        delete_tree(root);
        root = build_tree(points_ptrs, 0, DIMS);
        std::cout << "Tree rebuilt for epsilon = " << epsilon << "\n";
    }

    std::cout << "Coverage: red>=" << req_red << " blue>=" << req_blue << "\n";
    
    auto q0 = std::chrono::high_resolution_clock::now();
    auto res = queryBestRT_cov(dataset, points_data, root, epsilon, s, en, a, b, req_red, req_blue, cumulative_red, cumulative_blue);
    auto q1 = std::chrono::high_resolution_clock::now();

    if (!res) std::cout << "Best range = [NA,NA] (similarity = 0.0)\n";
    else std::cout << "Best range = [" << res->min_val << "," << res->max_val << "] (similarity = " << res->tversky << ")\n";
    std::cout << "Query executed in " << std::chrono::duration_cast<std::chrono::milliseconds>(q1-q0).count() << " ms\n";
  }

  delete_tree(root);
  return 0;
}
