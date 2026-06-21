#include "../query/rtree_cov.h"

int main(int argc, char *argv[]) {
    if (argc < 3) { cout << "Usage: main_rtree_cov <input> <num_colors>\n"; return 1; }
    string csvFile = argv[1];
    int numberOfColors = stoi(argv[2]);

    while (loadConstraintsFromConsole()) {
    auto t0=chrono::high_resolution_clock::now();
    Data data;
    try { data = readCSV(csvFile); } catch (const exception &e) { cerr << e.what() << "\n"; return 1; }
    if (data.m == 0) { cout << "No pairwise columns found.\n"; return 1; }
    if ((int)data.uniqueColors.size() != numberOfColors) { cout << "Color mismatch\n"; return 1; }
    t0 = chrono::high_resolution_clock::now();

    vector<string> colorOrder(data.uniqueColors.begin(), data.uniqueColors.end());
    cout << "Colors: "; for (auto &c : colorOrder) cout << c << " "; cout << "\n";

    CoverageData cov;
    try { cov = buildCoverageData(csvFile, colorOrder); }
    catch (const exception &e) { cerr << e.what() << "\n"; return 1; }

    RTree tree(data.points, data.m + 1);
    auto t1 = chrono::high_resolution_clock::now();

    cout << "R-Tree Coverage mode. Built in " << fixed << setprecision(4) << (chrono::duration<double, std::milli>(t1-t0).count() + data.pointsBuildTimeMs) << " ms\n";
    cout << "n=" << data.n << ", m=" << data.m << "\n";

    string line;
    while (true) {
        cout << "Enter epsilon start end alpha beta";
        for (auto &c : colorOrder) cout << " min_" << c;
        cout << " (-1 to exit):\n";
        if (!getline(cin, line)) break;
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;
        istringstream iss(line);
        double e, s, en, a, b;
        if (!(iss >> e >> s >> en >> a >> b)) continue;
        if (e == -1) break;
        int eps = (int)e;

        vector<int> required(numberOfColors); bool ok = true;
        for (int i = 0; i < numberOfColors; ++i)
            if (!(iss >> required[i])) { ok = false; break; }
        if (!ok) { cout << "Enter coverage for all " << numberOfColors << " colors.\n"; continue; }

        cout << "Coverage: ";
        for (int i = 0; i < numberOfColors; ++i)
            cout << colorOrder[i] << ">=" << required[i] << " ";
        cout << "\n";

        auto q0 = chrono::high_resolution_clock::now();
        auto res = queryBestRT_rtree_cov(data, tree, cov, eps, s, en, a, b, required);
        auto q1 = chrono::high_resolution_clock::now();

        if (!res) cout << "Best range = [NA,NA] (similarity = 0.0)\n";
        else cout << "Best range = [" << res->leftVal << "," << res->rightVal
                  << "] (similarity = " << res->similarity << ")\n";
        cout << "Query executed in " << fixed << setprecision(4) << chrono::duration<double, std::milli>(q1-q0).count() << " ms\n";
    }
    }
    return 0;
}
