#include "../common/fair_range.h"
#include <iostream>

void find_best_fair_range_brute_force(
    int query_i, int query_j, double epsilon,
    const std::vector<double> &cumulative_red,
    const std::vector<double> &cumulative_blue) {

  int best_i = -1;
  int best_j = -1;
  double max_similarity = -1.0;
  const int n = cumulative_red.size();
  double lower_bound = 1.0 / (1.0 + epsilon);
  double upper_bound = 1.0 + epsilon;

  for (int i = 0; i < n; ++i) {
    for (int j = i; j < n; ++j) {
      double red_score =
          cumulative_red[j] - (i > 0 ? cumulative_red[i - 1] : 0.0);
      double blue_score =
          cumulative_blue[j] - (i > 0 ? cumulative_blue[i - 1] : 0.0);

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
        double similarity = calculate_tversky_index(query_i, query_j, i, j);
        if (similarity > max_similarity) {
          max_similarity = similarity;
          best_i = i;
          best_j = j;
        }
      }
    }
  }

  std::cout << "\n--- Brute-Force Results ---" << std::endl;
  if (best_j != -1) {
    std::cout << "Best Fair Range Found Index: [" << best_i << ", " << best_j
              << "]" << std::endl;
    std::cout << "Tversky Index: " << max_similarity << std::endl;
  } else {
    std::cout << "No fair range could be found using the brute-force method."
              << std::endl;
  }
}

void find_best_fair_range_brute_force_cov(
    int query_i, int query_j, double epsilon, double alpha, double beta,
    double req_red, double req_blue,
    const std::vector<double> &cumulative_red,
    const std::vector<double> &cumulative_blue) {

  int best_i = -1;
  int best_j = -1;
  double max_similarity = -1.0;
  const int n = cumulative_red.size();
  double lower_bound = 1.0 / (1.0 + epsilon);
  double upper_bound = 1.0 + epsilon;

  for (int i = 0; i < n; ++i) {
    for (int j = i; j < n; ++j) {
      double red_score =
          cumulative_red[j] - (i > 0 ? cumulative_red[i - 1] : 0.0);
      double blue_score =
          cumulative_blue[j] - (i > 0 ? cumulative_blue[i - 1] : 0.0);

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

      if (is_fair && red_score >= req_red && blue_score >= req_blue) {
        double similarity = calculate_tversky_index(query_i, query_j, i, j, alpha, beta);
        if (similarity > max_similarity) {
          max_similarity = similarity;
          best_i = i;
          best_j = j;
        } else if (similarity == max_similarity && similarity >= 0.0) {
          int wBest = best_j - best_i;
          int wCur = j - i;
          if (wCur < wBest || (wCur == wBest && i < best_i)) {
            best_i = i;
            best_j = j;
          }
        }
      }
    }
  }

  std::cout << "\n--- Brute-Force Results ---" << std::endl;
  if (best_j != -1) {
    std::cout << "Best Fair Range Found Index: [" << best_i << ", " << best_j
              << "]" << std::endl;
    std::cout << "Tversky Index: " << max_similarity << std::endl;
  } else {
    std::cout << "No fair range could be found using the brute-force method."
              << std::endl;
  }
}