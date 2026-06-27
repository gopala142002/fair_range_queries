#include "data_2d.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

// Assuming config.h / common.h diffPairs and ratioPairs are correctly linked.
extern vector<DiffPairConfig> diffPairs;
extern vector<RatioPairConfig> ratioPairs;

RawData2D readLiveData2D(const string& filename, int dimX, int dimY) {
    RawData2D raw;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open " << filename << endl;
        return raw;
    }

    string line;
    if (!getline(file, line)) return raw;
    int numPoints = stoi(line);

    if (!getline(file, line)) return raw;
    // skip "5 Dims"

    vector<vector<double>> coords(numPoints);
    for (int i = 0; i < numPoints; ++i) {
        if (!getline(file, line)) break;
        stringstream ss(line);
        double val;
        while (ss >> val) {
            coords[i].push_back(val);
        }
    }

    if (!getline(file, line)) {
        cerr << "Missing colors line!" << endl;
        return raw;
    }
    stringstream ss(line);
    string color;
    vector<string> colors;
    while (ss >> color) {
        colors.push_back(color);
    }

    set<string> uniqueC;
    for (int i = 0; i < numPoints; ++i) {
        RawPoint p;
        p.x = coords[i][dimX];
        p.y = coords[i][dimY];
        p.color = colors[i];
        p.original_index = i;
        raw.pts.push_back(p);
        uniqueC.insert(p.color);
    }

    raw.md = diffPairs.size();
    raw.mr = ratioPairs.size();
    raw.uniqueColorsList.assign(uniqueC.begin(), uniqueC.end());

    return raw;
}

Data buildSubsetData(const vector<RawPoint>& subset, const RawData2D& raw) {
    Data data;
    int n = subset.size();
    data.n = n;
    data.md = raw.md;
    data.mr = raw.mr;
    data.m = data.md + 2 * data.mr;

    map<string, int> colorIndex;
    for (int i = 0; i < (int)raw.uniqueColorsList.size(); ++i) {
        colorIndex[raw.uniqueColorsList[i]] = i;
        data.uniqueColors.insert(raw.uniqueColorsList[i]);
    }
    int numColors = raw.uniqueColorsList.size();

    data.prefix.assign(numColors, vector<int>(n + 1, 0));
    data.keys.resize(n);

    for (int t = 0; t < n; ++t) {
        data.keys[t] = subset[t].y; // y is the 1D key
        for (int c = 0; c < numColors; ++c) {
            data.prefix[c][t + 1] = data.prefix[c][t];
        }
        auto it = colorIndex.find(subset[t].color);
        if (it != colorIndex.end()) {
            data.prefix[it->second][t + 1]++;
        }
    }

    data.points.assign(n + 1, vector<double>(data.m + 1, 0.0));
    for (int t = 0; t <= n; ++t) {
        int dim = 0;

        for (const auto &config : diffPairs) {
            int idxA = colorIndex.count(config.colorA) ? colorIndex[config.colorA] : -1;
            int idxB = colorIndex.count(config.colorB) ? colorIndex[config.colorB] : -1;

            double countA = (idxA != -1) ? data.prefix[idxA][t] : 0;
            double countB = (idxB != -1) ? data.prefix[idxB][t] : 0;

            data.points[t][dim++] = config.weightA * countA - config.weightB * countB;
        }

        for (const auto &config : ratioPairs) {
            int idxA = colorIndex.count(config.colorA) ? colorIndex[config.colorA] : -1;
            int idxB = colorIndex.count(config.colorB) ? colorIndex[config.colorB] : -1;

            double countA = (idxA != -1) ? data.prefix[idxA][t] : 0;
            double countB = (idxB != -1) ? data.prefix[idxB][t] : 0;

            double cr = config.weightA * countA - config.weightB * countB * (1.0 + config.delta);
            double cb = config.weightB * countB * (1.0 - config.delta) - config.weightA * countA;

            data.points[t][dim++] = cr;
            data.points[t][dim++] = cb;
        }

        data.points[t][dim] = (double)t;
    }

    return data;
}
