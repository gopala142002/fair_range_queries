#pragma once
#include "../trees/range_tree.h"

// Optimized Range Tree query with Tversky-based pruning and smart side selection.
optional<Result> queryBestRT_opt(Data &data, RangeTree &tree,
                                 double startVal, double endVal,
                                 double alpha, double beta);
