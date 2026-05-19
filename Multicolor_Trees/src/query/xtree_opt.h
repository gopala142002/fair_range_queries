#pragma once
#include "../trees/xtree.h"

// X-tree query with Tversky-based pruning and smart side selection.
optional<Result> queryBestXT_opt(Data &data, XTree &tree,
                                  int epsilon, double startVal, double endVal,
                                  double alpha, double beta);
