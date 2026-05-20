#ifndef X_TREE_OPT_H
#define X_TREE_OPT_H

#include "../common/fair_range.h"
#include "../trees/X_tree_Synthetic.h" // Matches your provided XTree header
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
queryBestXT_opt(const std::vector<DatasetPoint> &dataset,
                const std::vector<Point> &points_data, XTreeNode *tree_root,
                double epsilon, double startVal, double endVal, double alpha,
                double beta);

#endif // X_TREE_OPT_H