#pragma once
#include "../common/common.h"
#include "data_2d.h"
#include "../trees/kd_tree.h"
#include <optional>
#include <vector>

struct Result2D {
    double min_x, max_x, min_y, max_y, similarity;
};

std::optional<Result2D> queryBestKD_opt_2d_yx(
    RawData2D &raw,
    double qX_min, double qX_max,
    double qY_min, double qY_max,
    double alpha, double beta);
