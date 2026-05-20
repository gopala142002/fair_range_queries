#ifndef FAIR_RANGE_H
#define FAIR_RANGE_H

#include <string>
#include <vector>

// Core data structure
struct DatasetPoint {
  double value;
  std::string color;
  int id;
  DatasetPoint(double v, std::string c, int i) : value(v), color(c), id(i) {}
};

// Globals for Tree Instrumentation (Shared across files)
extern long long search_nodes_visited;
extern long long search_points_considered;

// Shared Functions
std::vector<DatasetPoint> read_custom_dataset(const std::string &filename);
double calculate_tversky_index(int qi, int qj, int i, int j, double alpha = 0.5,
                               double beta = 0.5);

// Algorithm Declarations
void find_best_fair_range_bfs(int query_i, int query_j, double epsilon,
                              const std::vector<double> &cumulative_red,
                              const std::vector<double> &cumulative_blue,
                              const std::vector<DatasetPoint> &dataset);

void find_best_fair_range_brute_force(
    int query_i, int query_j, double epsilon,
    const std::vector<double> &cumulative_red,
    const std::vector<double> &cumulative_blue);

void find_best_fair_range_brute_force_cov(
    int query_i, int query_j, double epsilon, double alpha, double beta,
    double req_red, double req_blue,
    const std::vector<double> &cumulative_red,
    const std::vector<double> &cumulative_blue);

#endif // FAIR_RANGE_H