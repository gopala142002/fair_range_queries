#include "data_2d.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

extern vector<DiffPairConfig> diffPairs;
extern vector<RatioPairConfig> ratioPairs;


// ============================================================
// READ 2D DATASET
//
// Format:
//
// 30000
// 2 Dims + Color
// x y color
// x y color
// ...
// ============================================================

RawData2D readLiveData2D(
    const string& filename,
    int dimX,
    int dimY
) {
    RawData2D raw;

    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Failed to open " << filename << endl;
        return raw;
    }


    // Dataset contains exactly 2 dimensions: 0 and 1
    if (dimX < 0 || dimX >= 2 ||
        dimY < 0 || dimY >= 2) {

        cerr << "Invalid dimensions: "
             << dimX << " " << dimY << endl;

        return raw;
    }


    string line;


    // --------------------------------------------------------
    // First line: number of points
    // --------------------------------------------------------

    if (!getline(file, line))
        return raw;

    int numPoints = stoi(line);


    // --------------------------------------------------------
    // Second line: "2 Dims + Color"
    // --------------------------------------------------------

    if (!getline(file, line))
        return raw;


    set<string> uniqueC;

    raw.pts.reserve(numPoints);


    // --------------------------------------------------------
    // Each row:
    //
    // coordinate_0 coordinate_1 color
    // --------------------------------------------------------

    for (int i = 0; i < numPoints; ++i) {

        if (!getline(file, line)) {
            cerr << "Dataset ended early at point "
                 << i << endl;
            break;
        }


        stringstream ss(line);

        double coord0;
        double coord1;
        string color;


        if (!(ss >> coord0 >> coord1 >> color)) {

            cerr << "Invalid row at point "
                 << i << ": "
                 << line << endl;

            continue;
        }


        double coords[2] = {
            coord0,
            coord1
        };


        RawPoint p;

        p.x = coords[dimX];
        p.y = coords[dimY];
        p.color = color;
        p.original_index = i;


        raw.pts.push_back(p);

        uniqueC.insert(color);
    }


    raw.md = diffPairs.size();
    raw.mr = ratioPairs.size();

    raw.uniqueColorsList.assign(
        uniqueC.begin(),
        uniqueC.end()
    );


    return raw;
}


// ============================================================
// BUILD SUBSET DATA
// ============================================================

Data buildSubsetData(
    const vector<RawPoint>& subset,
    const RawData2D& raw
) {
    Data data;

    int n = subset.size();

    data.n = n;
    data.md = raw.md;
    data.mr = raw.mr;
    data.m = data.md + 2 * data.mr;


    // --------------------------------------------------------
    // Color mapping
    // --------------------------------------------------------

    map<string, int> colorIndex;


    for (int i = 0;
         i < (int)raw.uniqueColorsList.size();
         ++i) {

        colorIndex[raw.uniqueColorsList[i]] = i;

        data.uniqueColors.insert(
            raw.uniqueColorsList[i]
        );
    }


    int numColors =
        raw.uniqueColorsList.size();


    // --------------------------------------------------------
    // Prefix arrays
    // --------------------------------------------------------

    data.prefix.assign(
        numColors,
        vector<int>(n + 1, 0)
    );


    data.keys.resize(n);


    for (int t = 0; t < n; ++t) {

        data.keys[t] = subset[t].y;


        for (int c = 0; c < numColors; ++c) {

            data.prefix[c][t + 1] =
                data.prefix[c][t];
        }


        auto it =
            colorIndex.find(subset[t].color);


        if (it != colorIndex.end()) {

            data.prefix[it->second][t + 1]++;
        }
    }


    // --------------------------------------------------------
    // Build fairness-space points
    // --------------------------------------------------------

    data.points.assign(
        n + 1,
        vector<double>(data.m + 1, 0.0)
    );


    for (int t = 0; t <= n; ++t) {

        int dim = 0;


        // ====================================================
        // Difference constraints
        // ====================================================

        for (const auto& config : diffPairs) {

            int idxA =
                colorIndex.count(config.colorA)
                ? colorIndex[config.colorA]
                : -1;


            int idxB =
                colorIndex.count(config.colorB)
                ? colorIndex[config.colorB]
                : -1;


            double countA =
                (idxA != -1)
                ? data.prefix[idxA][t]
                : 0;


            double countB =
                (idxB != -1)
                ? data.prefix[idxB][t]
                : 0;


            data.points[t][dim++] =
                config.weightA * countA
                -
                config.weightB * countB;
        }


        // ====================================================
        // Ratio constraints
        // ====================================================

        for (const auto& config : ratioPairs) {

            int idxA =
                colorIndex.count(config.colorA)
                ? colorIndex[config.colorA]
                : -1;


            int idxB =
                colorIndex.count(config.colorB)
                ? colorIndex[config.colorB]
                : -1;


            double countA =
                (idxA != -1)
                ? data.prefix[idxA][t]
                : 0;


            double countB =
                (idxB != -1)
                ? data.prefix[idxB][t]
                : 0;


            double cr =
                config.weightA * countA
                -
                config.weightB
                * countB
                * (1.0 + config.delta);


            double cb =
                config.weightB
                * countB
                * (1.0 - config.delta)
                -
                config.weightA * countA;


            data.points[t][dim++] = cr;

            data.points[t][dim++] = cb;
        }


        // Index dimension
        data.points[t][dim] =
            (double)t;
    }


    return data;
}