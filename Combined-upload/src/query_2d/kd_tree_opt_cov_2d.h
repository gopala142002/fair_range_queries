#ifndef KD_TREE_OPT_COV_2D_H
#define KD_TREE_OPT_COV_2D_H

#include "kd_tree_opt_2d.h"

#include <optional>
#include <vector>

std::optional<Result2D> queryBestKD_opt_cov_2d(
    RawData2D &raw,
    double qX_min,
    double qX_max,
    double qY_min,
    double qY_max,
    double alpha,
    double beta,
    const std::vector<int> &required
);

#endif