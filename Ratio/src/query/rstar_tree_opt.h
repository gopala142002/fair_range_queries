#ifndef RSTAR_TREE_OPT_H
#define RSTAR_TREE_OPT_H

#include "../common/fair_range.h"
#include "../trees/R_Star_Synthetic.h"
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
queryBestRT_rstar_opt(const std::vector<DatasetPoint> &dataset,
                      const std::vector<Point> &points_data,
                      RStarTreeNode *tree_root, // Changed from RStarTree*
                      double epsilon, double startVal, double endVal,
                      double alpha, double beta);

#endif // RSTAR_TREE_OPT_H