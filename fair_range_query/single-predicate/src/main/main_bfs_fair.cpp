#include "../bfs/bfs_fair.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Usage: main_bfs_fair <input> <num_colors>\n";
        return 1;
    }
    string csvFile     = argv[1];
    int numberOfColors = stoi(argv[2]);

    while (loadConstraintsFromConsole()) {
    auto t0=chrono::high_resolution_clock::now();
    Data data;
    try { data = readCSV(csvFile); }
    catch (const exception &e) { cerr << e.what() << "\n"; return 1; }

    if (data.m == 0) { cout << "No pairwise columns found.\n"; return 1; }
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

    auto t1 = chrono::high_resolution_clock::now();
    cout << "Data loaded in "
         << fixed << setprecision(4) << (chrono::duration<double, std::milli>(t1-t0).count() + data.pointsBuildTimeMs)
         << " ms\n";
    cout << "BFS Fairness mode. n=" << data.n << ", m=" << data.m << "\n";

    string line;
    while (true) {
        // Same format as trees: epsilon start end alpha beta
        cout << "Enter epsilon start end alpha beta (-1 to exit):\n";

        if (!getline(cin, line)) break;
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;

        istringstream iss(line);
        double eps_d, s, en, a, b;
        if (!(iss >> eps_d >> s >> en >> a >> b)) continue;
        if (eps_d == -1) break;
        int eps = (int)eps_d;

        auto q0 = chrono::high_resolution_clock::now();
        auto res = bfsFair(data, cov, s, en, eps, a, b);
        auto q1 = chrono::high_resolution_clock::now();

        if (!res)
            cout << "No fair range found.\n";
        else
            cout << "Best range = [" << res->leftVal << "," << res->rightVal
                 << "] (similarity = " << res->similarity << ")\n";

        cout << "Query executed in "
             << fixed << setprecision(4) << chrono::duration<double, std::milli>(q1-q0).count()
             << " ms\n";
    }
    }
    return 0;
}
