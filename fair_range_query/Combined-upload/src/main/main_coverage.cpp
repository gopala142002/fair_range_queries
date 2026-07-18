#include "../query/kd_tree_cov.h"
#include "../query/range_tree_cov.h"
#include "../query/rtree_cov.h"
#include "../query/rstar_tree_cov.h"
#include "../query/xtree_cov.h"
#include "../query/kd_tree_opt_cov.h"
#include "../query/range_tree_opt_cov.h"
#include "../query/rtree_opt_cov.h"
#include "../query/rstar_tree_opt_cov.h"
#include "../query/xtree_opt_cov.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cout << "Usage: main_coverage <input> <num_colors> <mode>\n";
        cout << "Modes: kd, kd_opt, range, range_opt, rtree, rtree_opt,\n";
        cout << "       rstar, rstar_opt, xtree, xtree_opt\n";
        return 1;
    }
    string csvFile     = argv[1];
    int numberOfColors = stoi(argv[2]);
    string mode        = argv[3];

    if (mode != "kd" && mode != "kd_opt" &&
        mode != "range" && mode != "range_opt" &&
        mode != "rtree" && mode != "rtree_opt" &&
        mode != "rstar" && mode != "rstar_opt" &&
        mode != "xtree" && mode != "xtree_opt") {
        cout << "Unknown mode: " << mode << "\n";
        cout << "Available modes: kd, kd_opt, range, range_opt, rtree, rtree_opt,\n";
        cout << "                 rstar, rstar_opt, xtree, xtree_opt\n";
        return 1;
    }

    while (loadConstraintsFromConsole()) {
    auto t0=chrono::high_resolution_clock::now();
    Data data;
    try { data = readCSV(csvFile); }
    catch (const exception &e) { cerr << e.what() << "\n"; return 1; }
    if ((int)data.uniqueColors.size() != numberOfColors) {
        cout << "Color mismatch: file has " << data.uniqueColors.size() << "\n";
        return 1;
    }
    t0 = chrono::high_resolution_clock::now();

    vector<string> colorOrder(data.uniqueColors.begin(), data.uniqueColors.end());
    cout << "Colors: "; for (auto &c : colorOrder) cout << c << " "; cout << "\n";

    CoverageData cov;
    try { cov = buildCoverageData(csvFile, colorOrder); }
    catch (const exception &e) { cerr << e.what() << "\n"; return 1; }

    // Build the appropriate tree
    KDTree    *kdTree    = nullptr;
    RangeTree *rangeTree = nullptr;
    RTree     *rTree     = nullptr;
    RStarTree *rstarTree = nullptr;
    XTree     *xTree     = nullptr;

    string modeName;
    if (mode == "kd" || mode == "kd_opt") {
        kdTree = new KDTree(data.points, data.m + 1);
        modeName = (mode == "kd") ? "KD-Tree Coverage" : "KD-Tree Coverage Optimized";
    } else if (mode == "range" || mode == "range_opt") {
        rangeTree = new RangeTree(data.points, data.m + 1);
        modeName = (mode == "range") ? "Range Tree Coverage" : "Range Tree Coverage Optimized";
    } else if (mode == "rtree" || mode == "rtree_opt") {
        rTree = new RTree(data.points, data.m + 1);
        modeName = (mode == "rtree") ? "R-Tree Coverage" : "R-Tree Coverage Optimized";
    } else if (mode == "rstar" || mode == "rstar_opt") {
        rstarTree = new RStarTree(data.points, data.m + 1);
        modeName = (mode == "rstar") ? "R* Tree Coverage" : "R* Tree Coverage Optimized";
    } else if (mode == "xtree" || mode == "xtree_opt") {
        xTree = new XTree(data.points, data.m + 1);
        modeName = (mode == "xtree") ? "X-Tree Coverage" : "X-Tree Coverage Optimized";
    }

    auto t1 = chrono::high_resolution_clock::now();
    cout << modeName << " mode. Built in "
         << fixed << setprecision(4) << (chrono::duration<double, std::milli>(t1-t0).count() + data.pointsBuildTimeMs) << " ms\n";
    cout << "n=" << data.n << ", m=" << data.m << "\n";

    string line;
    while (true) {
        cout << "Enter start end alpha beta";
        for (auto &c : colorOrder) cout << " min_" << c;
        cout << " (-1 to exit):\n";
        if (!getline(cin, line)) break;
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;
        istringstream iss(line);
        double s, en, a, b;
        if (!(iss >> s >> en >> a >> b)) continue;
        if (s == -1) break;

        vector<int> required(numberOfColors);
        bool ok = true;
        for (int i = 0; i < numberOfColors; ++i)
            if (!(iss >> required[i])) { ok = false; break; }
        if (!ok) {
            cout << "Enter coverage for all " << numberOfColors << " colors.\n";
            continue;
        }
        cout << "Coverage: ";
        for (int i = 0; i < numberOfColors; ++i)
            cout << colorOrder[i] << ">=" << required[i] << " ";
        cout << "\n";

        auto q0 = chrono::high_resolution_clock::now();
        optional<Result> res;

        if      (mode == "kd")         res = queryBestKD_cov(data, *kdTree, cov, s, en, a, b, required);
        else if (mode == "kd_opt")     res = queryBestKD_opt_cov(data, *kdTree, cov, s, en, a, b, required);
        else if (mode == "range")      res = queryBestRT_cov(data, *rangeTree, cov, s, en, a, b, required);
        else if (mode == "range_opt")  res = queryBestRT_opt_cov(data, *rangeTree, cov, s, en, a, b, required);
        else if (mode == "rtree")      res = queryBestRT_rtree_cov(data, *rTree, cov, s, en, a, b, required);
        else if (mode == "rtree_opt")  res = queryBestRT_rtree_opt_cov(data, *rTree, cov, s, en, a, b, required);
        else if (mode == "rstar")      res = queryBestRT_rstar_cov(data, *rstarTree, cov, s, en, a, b, required);
        else if (mode == "rstar_opt")  res = queryBestRT_rstar_opt_cov(data, *rstarTree, cov, s, en, a, b, required);
        else if (mode == "xtree")      res = queryBestXT_cov(data, *xTree, cov, s, en, a, b, required);
        else if (mode == "xtree_opt")  res = queryBestXT_opt_cov(data, *xTree, cov, s, en, a, b, required);

        auto q1 = chrono::high_resolution_clock::now();

        if (!res) cout << "Best range = [NA,NA] (similarity = 0.0)\n";
        else cout << "Best range = [" << res->leftVal << "," << res->rightVal
                  << "] (similarity = " << res->similarity << ")\n";
        cout << "Query executed in "
             << fixed << setprecision(4) << chrono::duration<double, std::milli>(q1-q0).count() << " ms\n";
    }

    delete kdTree;
    delete rangeTree;
    delete rTree;
    delete rstarTree;
    delete xTree;

    }
    return 0;
}
