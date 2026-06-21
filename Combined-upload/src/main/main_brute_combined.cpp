#include "../brute/brute.h"
#include "../brute/brute_fair.h"
#include "../brute/brute_fair_cov.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cout << "Usage: main_brute <input> <num_colors> <mode>\n";
        cout << "Modes: base, fair, fair_cov\n";
        return 1;
    }
    string csvFile     = argv[1];
    int numberOfColors = stoi(argv[2]);
    string mode        = argv[3];

    if (mode != "base" && mode != "fair" && mode != "fair_cov") {
        cout << "Unknown mode: " << mode << "\n";
        cout << "Available modes: base, fair, fair_cov\n";
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

    // Build coverage data for fair and fair_cov modes
    vector<string> colorOrder;
    CoverageData cov;
    if (mode == "fair" || mode == "fair_cov") {
        colorOrder.assign(data.uniqueColors.begin(), data.uniqueColors.end());
        cout << "Colors: "; for (auto &c : colorOrder) cout << c << " "; cout << "\n";
        try { cov = buildCoverageData(csvFile, colorOrder); }
        catch (const exception &e) { cerr << e.what() << "\n"; return 1; }
    }

    auto t1 = chrono::high_resolution_clock::now();

    string modeName;
    if      (mode == "base")     modeName = "Brute Force";
    else if (mode == "fair")     modeName = "Brute Force Fair";
    else if (mode == "fair_cov") modeName = "Brute Force Fair+Coverage";

    cout << modeName << " mode. Built in "
         << fixed << setprecision(4) << (chrono::duration<double, std::milli>(t1-t0).count() + data.pointsBuildTimeMs) << " ms\n";
    cout << "n=" << data.n << ", m=" << data.m << "\n";

    string line;
    while (true) {
        if (mode == "fair_cov") {
            cout << "Enter start end alpha beta";
            for (auto &c : colorOrder) cout << " min_" << c;
            cout << " (-1 to exit):\n";
        } else {
            cout << "Enter start end alpha beta (-1 to exit):\n";
        }

        if (!getline(cin, line)) break;
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;
        istringstream iss(line);
        double s, en, a, b;
        if (!(iss >> s >> en >> a >> b)) continue;
        if (s == -1) break;

        if (mode == "base") {
            auto q0 = chrono::high_resolution_clock::now();
            auto res = queryBrute(data, s, en, a, b);
            auto q1 = chrono::high_resolution_clock::now();
            if (!res) cout << "Best range = [NA,NA] (similarity = 0.0)\n";
            else cout << "Best range = [" << res->leftVal << "," << res->rightVal
                      << "] (similarity = " << res->similarity << ")\n";
            cout << "Query executed in "
                 << fixed << setprecision(4) << chrono::duration<double, std::milli>(q1-q0).count() << " ms\n";

        } else if (mode == "fair") {
            auto q0 = chrono::high_resolution_clock::now();
            auto res = queryBruteFair(data, s, en, a, b);
            auto q1 = chrono::high_resolution_clock::now();
            if (!res) cout << "No fair range found.\n";
            else cout << "Best range = [" << res->leftVal << "," << res->rightVal
                      << "] (similarity = " << res->similarity << ")\n";
            cout << "Query executed in "
                 << fixed << setprecision(4) << chrono::duration<double, std::milli>(q1-q0).count() << " ms\n";

        } else {
            // fair_cov mode — read coverage requirements
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
            auto res = queryBruteFairCov(data, cov, s, en, a, b, required);
            auto q1 = chrono::high_resolution_clock::now();
            if (!res) cout << "No fair+coverage range found.\n";
            else cout << "Best range = [" << res->leftVal << "," << res->rightVal
                      << "] (similarity = " << res->similarity << ")\n";
            cout << "Query executed in "
                 << fixed << setprecision(4) << chrono::duration<double, std::milli>(q1-q0).count() << " ms\n";
        }
    }
    }
    return 0;
}
