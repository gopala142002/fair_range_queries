#include "fair_range.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>


std::vector<DatasetPoint> read_custom_dataset(const std::string &filename) {
  std::vector<DatasetPoint> dataset;
  std::ifstream file_handler(filename);

  if (!file_handler.is_open()) {
    std::cerr << "Error: Failed to open file " << filename << std::endl;
    return dataset;
  }

  std::string line;
  while (std::getline(file_handler, line)) {
    if (line.empty())
      continue;

    // Convert commas to spaces
    std::replace(line.begin(), line.end(), ',', ' ');
    std::stringstream ss(line);
    double value;
    int category;

    if (ss >> value >> category) {
      std::string color_str = (category == 1) ? "blue" : "red";
      dataset.emplace_back(value, color_str, 0);
    }
  }

  std::sort(dataset.begin(), dataset.end(),
            [](const DatasetPoint &a, const DatasetPoint &b) {
              return a.value < b.value;
            });

  for (size_t i = 0; i < dataset.size(); ++i) {
    dataset[i].id = static_cast<int>(i);
  }

  return dataset;
}