#pragma once
#include "data_2d.h"
#include <optional>

struct Result2D; // Already defined in kd_tree_opt_2d.h, wait we need to include it or redefine it. Let's include kd_tree_opt_2d.h

#include "kd_tree_opt_2d.h"

std::optional<Result2D> brute_force_2d(
    RawData2D &raw,
    double qX_min, double qX_max,
    double qY_min, double qY_max,
    double alpha, double beta);
