#include "../query2d/data_2d.h"
#include "../query2d/kd_tree_opt_2d_yx.h"
#include "../common/common.h"
#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

// Provide global definitions for config pairs (already in common.cpp, so we don't need to re-define if they are extern, wait, common.cpp defines them without extern but they are used in data_2d.cpp with extern.)

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <live_l2_5d_30k.txt> <qX_min> <qX_max> <qY_min> <qY_max>\n";
        return 1;
    }

    string filename = argv[1];
    double qX_min = stod(argv[2]);
    double qX_max = stod(argv[3]);
    double qY_min = stod(argv[4]);
    double qY_max = stod(argv[5]);
    
    // We can pick dimension 0 and 1 as X and Y
    int dimX = 0, dimY = 1;

    // Load configs from console as required by existing common.cpp logic
    if (!loadConstraintsFromConsole()) {
        cerr << "Constraints load failed.\n";
        return 1;
    }

    RawData2D raw = readLiveData2D(filename, dimX, dimY);
    if (raw.pts.empty()) {
        cerr << "Failed to load data or dataset is empty.\n";
        return 1;
    }

    // cout << "Loaded " << raw.pts.size() << " points.\n";

    double alpha = 1.0, beta = 1.0;
    
    auto res = queryBestKD_opt_2d_yx(raw, qX_min, qX_max, qY_min, qY_max, alpha, beta);

    if (res.has_value()) {
        cout << fixed << setprecision(6);
        cout << res->similarity << "\n";
        cout << "X: [" << res->min_x << ", " << res->max_x << "] Y: [" << res->min_y << ", " << res->max_y << "]\n";
    } else {
        cout << "0.0\n";
    }

    return 0;
}
