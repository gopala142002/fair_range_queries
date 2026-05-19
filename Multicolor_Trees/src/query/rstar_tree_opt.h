#pragma once
#include "../trees/rstar_tree.h"

// R* tree query with Tversky-based pruning and smart side selection.
optional<Result> queryBestRT_rstar_opt(Data &data, RStarTree &tree,
                                       int epsilon, double startVal, double endVal,
                                       double alpha, double beta);
