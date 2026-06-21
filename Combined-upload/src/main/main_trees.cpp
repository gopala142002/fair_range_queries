#include "../trees/kd_tree.h"
#include "../trees/range_tree.h"
#include "../trees/rtree.h"
#include "../trees/rstar_tree.h"
#include "../trees/xtree.h"
#include "../query/kd_tree_opt.h"
#include "../query/range_tree_opt.h"
#include "../query/rtree_opt.h"
#include "../query/rstar_tree_opt.h"
#include "../query/xtree_opt.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cout << "Usage: main_trees <input> <num_colors> <mode>\n";
        cout << "Modes: kd, kd_opt, range, range_opt, rtree, rtree_opt,\n";
        cout << "       rstar, rstar_opt, xtree, xtree_opt\n";
        return 1;
    }
    string csvFile     = argv[1];
    int numberOfColors = stoi(argv[2]);
    string mode        = argv[3];

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

    // Build the appropriate tree
    KDTree    *kdTree    = nullptr;
    RangeTree *rangeTree = nullptr;
    RTree     *rTree     = nullptr;
    RStarTree *rstarTree = nullptr;
    XTree     *xTree     = nullptr;

    string modeName;
    if (mode == "kd" || mode == "kd_opt") {
        kdTree = new KDTree(data.points, data.m + 1);
        modeName = (mode == "kd") ? "KD-Tree" : "KD-Tree Optimized";
    } else if (mode == "range" || mode == "range_opt") {
        rangeTree = new RangeTree(data.points, data.m + 1);
        modeName = (mode == "range") ? "Range Tree" : "Range Tree Optimized";
    } else if (mode == "rtree" || mode == "rtree_opt") {
        rTree = new RTree(data.points, data.m + 1);
        modeName = (mode == "rtree") ? "R-Tree" : "R-Tree Optimized";
    } else if (mode == "rstar" || mode == "rstar_opt") {
        rstarTree = new RStarTree(data.points, data.m + 1);
        modeName = (mode == "rstar") ? "R* Tree" : "R* Tree Optimized";
    } else if (mode == "xtree" || mode == "xtree_opt") {
        xTree = new XTree(data.points, data.m + 1);
        modeName = (mode == "xtree") ? "X-Tree" : "X-Tree Optimized";
    } else {
        cout << "Unknown mode: " << mode << "\n";
        cout << "Available modes: kd, kd_opt, range, range_opt, rtree, rtree_opt,\n";
        cout << "                 rstar, rstar_opt, xtree, xtree_opt\n";
        return 1;
    }

    auto t1 = chrono::high_resolution_clock::now();
    cout << modeName << " mode. Built in "
         << fixed << setprecision(4) << (chrono::duration<double, std::milli>(t1-t0).count() + data.pointsBuildTimeMs) << " ms\n";
    cout << "n=" << data.n << ", m=" << data.m << "\n";

    string line;
    while (true) {
        cout << "Enter start end alpha beta (-1 to exit all):\n";
        if (!getline(cin, line)) break;
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;
        istringstream iss(line);
        double s, en, a, b;
        if (!(iss >> s >> en >> a >> b)) continue;
        if (s == -1) break;

        auto q0 = chrono::high_resolution_clock::now();
        optional<Result> res;

        if      (mode == "kd")         res = queryBestKD(data, *kdTree, s, en, a, b);
        else if (mode == "kd_opt")     res = queryBestKD_opt(data, *kdTree, s, en, a, b);
        else if (mode == "range")      res = queryBestRT(data, *rangeTree, s, en, a, b);
        else if (mode == "range_opt")  res = queryBestRT_opt(data, *rangeTree, s, en, a, b);
        else if (mode == "rtree")      res = queryBestRT_rtree(data, *rTree, s, en, a, b);
        else if (mode == "rtree_opt")  res = queryBestRT_rtree_opt(data, *rTree, s, en, a, b);
        else if (mode == "rstar")      res = queryBestRT_rstar(data, *rstarTree, s, en, a, b);
        else if (mode == "rstar_opt")  res = queryBestRT_rstar_opt(data, *rstarTree, s, en, a, b);
        else if (mode == "xtree")      res = queryBestXT(data, *xTree, s, en, a, b);
        else if (mode == "xtree_opt")  res = queryBestXT_opt(data, *xTree, s, en, a, b);

        auto q1 = chrono::high_resolution_clock::now();

        if (!res) cout << "[Tree] Best range = [NA,NA] (similarity = 0.0)\n";
        else cout << "[Tree] Best range = [" << res->leftVal << "," << res->rightVal
                  << "] (similarity = " << res->similarity << ")\n";
        cout << "[Tree] Query executed in "
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
