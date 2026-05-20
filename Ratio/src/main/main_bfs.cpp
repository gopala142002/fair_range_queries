#include "../common/fair_range.h"
#include <chrono>
#include <iostream>
#include <vector>

int main() {
  std::string filename = "data/synthetic_30k.csv";
  std::vector<DatasetPoint> dataset = read_custom_dataset(filename);

  if (dataset.empty()) {
    std::cerr << "Dataset could not be loaded or is empty." << std::endl;
    return 1;
  }
  const int DATASET_SIZE = dataset.size();
  std::cout << "Successfully loaded and sorted " << DATASET_SIZE << " records."
            << std::endl;

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

  double epsilon = 0.01;
  double start_val = 1.0;
  double end_val = 1000.0;

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
    std::cerr << "Error: No data points found in the specified range."
              << std::endl;
    return 1;
  }

  std::cout << "\nInitial Query Range (by index) is [" << query_i << ", "
            << query_j << "]" << std::endl;
  std::cout << "Initial Query Range (by values) is [" << dataset[query_i].value
            << ", " << dataset[query_j].value << "]" << std::endl;

  auto bfs_start = std::chrono::high_resolution_clock::now();

  find_best_fair_range_bfs(query_i, query_j, epsilon, cumulative_red,
                           cumulative_blue, dataset);

  auto bfs_end = std::chrono::high_resolution_clock::now();
  auto bfs_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      bfs_end - bfs_start);
  std::cout << "BFS search took: " << bfs_duration.count() << " ms"
            << std::endl;

  return 0;
}