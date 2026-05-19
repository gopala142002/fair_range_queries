#include "../bfs/bfs_fair_cov.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Usage: main_bfs_fair_cov <input> <num_colors>\n";
        cout << "Note: use original dataset (same as trees)\n";
        return 1;
    }
    string csvFile     = argv[1];
    int numberOfColors = stoi(argv[2]);

    auto t0 = chrono::high_resolution_clock::now();

    Data data;
    try { data = readCSV(csvFile); }
    catch (const exception &e) { cerr << e.what() << "\n"; return 1; }

    if (data.m == 0) { cout << "No pairwise columns found.\n"; return 1; }
    if ((int)data.uniqueColors.size() != numberOfColors) {
        cout << "Color mismatch: file has " << data.uniqueColors.size() << "\n";
        return 1;
    }

    // Build coverage data from color column — same as tree coverage programs
    vector<string> colorOrder(data.uniqueColors.begin(), data.uniqueColors.end());
    cout << "Colors: "; for (auto &c : colorOrder) cout << c << " "; cout << "\n";

    CoverageData cov;
    try { cov = buildCoverageData(csvFile, colorOrder); }
    catch (const exception &e) { cerr << e.what() << "\n"; return 1; }

    auto t1 = chrono::high_resolution_clock::now();
    cout << "Data loaded in "
         << chrono::duration_cast<chrono::milliseconds>(t1-t0).count()
         << " ms\n";
    cout << "BFS Fair+Coverage mode. n=" << data.n << ", m=" << data.m << "\n";

    string line;
    while (true) {
        // Same input format as tree programs:
        // epsilon start end alpha beta min_Color1 min_Color2 ...
        cout << "Enter epsilon start end alpha beta";
        for (auto &c : colorOrder) cout << " min_" << c;
        cout << " (-1 to exit):\n";

        if (!getline(cin, line)) break;
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;

        istringstream iss(line);
        double eps_d, s, en, a, b;
        if (!(iss >> eps_d >> s >> en >> a >> b)) continue;
        if (eps_d == -1) break;
        int eps = (int)eps_d;

        vector<int> required(numberOfColors);
        bool ok = true;
        for (int i = 0; i < numberOfColors; ++i)
            if (!(iss >> required[i])) { ok = false; break; }
        if (!ok) {
            cout << "Enter coverage for all " << numberOfColors << " colors.\n";
            continue;
        }

        cout << "Fairness epsilon: " << eps << "\n";
        cout << "Coverage: ";
        for (int i = 0; i < numberOfColors; ++i)
            cout << colorOrder[i] << ">=" << required[i] << " ";
        cout << "\n";

        auto q0 = chrono::high_resolution_clock::now();
        auto res = bfsFairCov(data, cov, s, en, eps, a, b, required);
        auto q1 = chrono::high_resolution_clock::now();

        if (!res)
            cout << "No fair+coverage range found.\n";
        else
            cout << "Best range = [" << res->leftVal << "," << res->rightVal
                 << "] (similarity = " << res->similarity << ")\n";

        cout << "Query executed in "
             << chrono::duration_cast<chrono::milliseconds>(q1-q0).count()
             << " ms\n";
    }
    return 0;
}
