#pragma once
#include "data_2d.h"
#include "kd_tree_opt_2d.h"

#include <optional>

std::optional<Result2D> bfsFair2D(
    RawData2D &raw,
    double qX_min,
    double qX_max,
    double qY_min,
    double qY_max,
    double alpha,
    double beta
);