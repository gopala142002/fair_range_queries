#ifndef RTREE_OPT_H
#define RTREE_OPT_H

#include "../common/fair_range.h"
#include "../trees/R_tree_Synthetic.h" // Matches your tree builder
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

std::optional<OptResult>
queryBestRT_rtree_opt(const std::vector<DatasetPoint> &dataset,
                      const std::vector<Point> &points_data,
                      RTreeNode *tree_root, double epsilon, double startVal,
                      double endVal, double alpha, double beta);

#endif // RTREE_OPT_H