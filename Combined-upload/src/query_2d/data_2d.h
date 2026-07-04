#pragma once
#include "../common/common.h"
#include <vector>
#include <string>

struct RawPoint {
    double x;
    double y;
    std::string color;
    int original_index;
};

struct RawData2D {
    std::vector<RawPoint> pts;
    int md;
    int mr;
    std::vector<std::string> uniqueColorsList;
};

RawData2D readLiveData2D(const std::string& filename, int dimX, int dimY);

// Build 1D Data subset for KD-tree from a sorted vector of RawPoints
Data buildSubsetData(const std::vector<RawPoint>& subset, const RawData2D& raw);
