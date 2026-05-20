#ifndef RANGE_TREE_COV_H
#define RANGE_TREE_COV_H

#include "../common/fair_range.h"
#include "../trees/range_tree_Synthetic.h"
#include <optional>
#include <vector>

#ifndef OPT_RESULT_DEFINED
#define OPT_RESULT_DEFINED
struct OptResult {
  double min_val;
  double max_val;
  double tversky;
};
#endif

int findCoveragePoint(int p, double req_red, double req_blue, 
                      const std::vector<double>& cum_red, const std::vector<double>& cum_blue);

void find_most_similar_fair_j_cov(RangeTreeNode *node, const QueryBox &Q,
                                  int query_i, int query_j, int current_i,
                                  int level, int total_levels,
                                  int coveragePoint, double alpha, double beta,
                                  double &max_similarity, int &best_j);

std::optional<OptResult>
queryBestRT_cov(const std::vector<DatasetPoint> &dataset,
                const std::vector<Point> &points_data, RangeTreeNode *tree_root,
                double epsilon, double startVal, double endVal, double alpha,
                double beta, double req_red, double req_blue,
                const std::vector<double>& cum_red, const std::vector<double>& cum_blue);

#endif // RANGE_TREE_COV_H
