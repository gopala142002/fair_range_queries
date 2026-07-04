#pragma once
#include "data_2d.h"
#include "../trees/kd_tree.h"
#include <optional>

struct Result2D {
    double x_min, x_max;
    double y_min, y_max;
    double similarity;
};

std::optional<Result2D> queryBestKD_opt_2d(
    RawData2D &raw,
    double qX_min, double qX_max,
    double qY_min, double qY_max,
    double alpha, double beta);
