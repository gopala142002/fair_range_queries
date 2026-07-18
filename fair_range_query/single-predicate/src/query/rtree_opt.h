#pragma once
#include "../trees/rtree.h"

// R-tree query with Tversky-based pruning and smart side selection.
optional<Result> queryBestRT_rtree_opt(Data &data, RTree &tree,
                                       double startVal, double endVal,
                                       double alpha, double beta);
