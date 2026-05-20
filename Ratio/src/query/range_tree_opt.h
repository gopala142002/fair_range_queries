#ifndef RANGE_TREE_OPT_H
#define RANGE_TREE_OPT_H

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

std::optional<OptResult>
queryBestRT_opt(const std::vector<DatasetPoint> &dataset,
                const std::vector<Point> &points_data, RangeTreeNode *tree_root,
                double epsilon, double startVal, double endVal, double alpha,
                double beta);

#endif // RANGE_TREE_OPT_H