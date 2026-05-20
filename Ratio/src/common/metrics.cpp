#include "fair_range.h"
#include <algorithm>

double calculate_tversky_index(int qi, int qj, int i, int j, double alpha,
                               double beta) {
  int intersection_start = std::max(qi, i);
  int intersection_end = std::min(qj, j);
  double intersection_size =
      std::max(0, intersection_end - intersection_start + 1);

  if (intersection_size == 0)
    return 0.0;

  double query_size = qj - qi + 1;
  double candidate_size = j - i + 1;

  double query_diff = query_size - intersection_size;
  double candidate_diff = candidate_size - intersection_size;

  return intersection_size /
         (intersection_size + alpha * query_diff + beta * candidate_diff);
}