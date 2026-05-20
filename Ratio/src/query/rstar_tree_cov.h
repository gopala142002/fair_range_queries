#ifndef RSTAR_TREE_COV_H
#define RSTAR_TREE_COV_H

#include "../common/fair_range.h"
#include "../trees/R_Star_Synthetic.h"
#include "range_tree_cov.h"
#include <optional>
#include <vector>

void find_most_similar_fair_j_rstar_tree_cov(RStarTreeNode *node, const QueryBox &Q,
                                     int query_i, int query_j, int current_i,
                                     int coveragePoint, double alpha, double beta,
                                     double &max_similarity, int &best_j);

std::optional<OptResult>
queryBestRStarTree_cov(const std::vector<DatasetPoint> &dataset,
                const std::vector<Point> &points_data, RStarTreeNode *tree_root,
                double epsilon, double startVal, double endVal, double alpha,
                double beta, double req_red, double req_blue,
                const std::vector<double>& cum_red, const std::vector<double>& cum_blue);

#endif
