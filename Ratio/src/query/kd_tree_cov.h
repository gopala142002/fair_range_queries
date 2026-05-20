#ifndef KD_TREE_COV_H
#define KD_TREE_COV_H

#include "../common/fair_range.h"
#include "../trees/KD_Tree_Synthetic.h"
#include "range_tree_cov.h"
#include <optional>
#include <vector>

void find_most_similar_fair_j_kdtree_cov(KDTreeNode *node, const QueryBox &Q,
                                     int query_i, int query_j, int current_i,
                                     int coveragePoint, double alpha, double beta,
                                     double &max_similarity, int &best_j);

std::optional<OptResult>
queryBestKDTree_cov(const std::vector<DatasetPoint> &dataset,
                const std::vector<Point> &points_data, KDTreeNode *tree_root,
                double epsilon, double startVal, double endVal, double alpha,
                double beta, double req_red, double req_blue,
                const std::vector<double>& cum_red, const std::vector<double>& cum_blue);

#endif
