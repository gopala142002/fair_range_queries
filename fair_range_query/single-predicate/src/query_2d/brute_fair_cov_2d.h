#ifndef BRUTE_FAIR_COV_2D_H
#define BRUTE_FAIR_COV_2D_H

#include "brute_2d.h"

#include <optional>
#include <vector>


std::optional<Result2D> brute_force_cov_2d(
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