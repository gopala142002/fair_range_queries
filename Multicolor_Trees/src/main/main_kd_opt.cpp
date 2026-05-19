#include "../query/kd_tree_opt.h"

int main(int argc, char *argv[]) {
    if (argc < 3) { cout << "Usage: main_kd_opt.cpp <input> <num_colors>\n"; return 1; }
    string csvFile = argv[1];
    int numberOfColors = stoi(argv[2]);
    auto t0 = chrono::high_resolution_clock::now();
    Data data;
    try { data = readCSV(csvFile); } catch (const exception &e) { cerr << e.what() << "\n"; return 1; }
    if (data.m==0) { cout << "No pairwise columns found.\n"; return 1; }
    if ((int)data.uniqueColors.size()!=numberOfColors) { cout << "Color mismatch\n"; return 1; }
    KDTree tree(data.points,data.m+1);
    auto t1 = chrono::high_resolution_clock::now();
    cout << "KD-Tree Optimized mode. Built in " << chrono::duration_cast<chrono::milliseconds>(t1-t0).count() << " ms\n";
    cout << "n=" << data.n << ", m=" << data.m << "\n";
    string line;
    while (true) {
        cout << "Enter epsilon start end alpha beta (-1 to exit all):\n";
        if (!getline(cin, line)) break;
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n")+1);
        if (line.empty()) continue;
        istringstream iss(line);
        double e, s, en, a, b;
        if (!(iss >> e >> s >> en >> a >> b)) continue;
        if (e==-1) break;
        int eps=(int)e;
        auto q0 = chrono::high_resolution_clock::now();
        auto res = queryBestKD_opt(data,tree,eps,s,en,a,b);
        auto q1 = chrono::high_resolution_clock::now();
        if (!res) cout << "Best range = [NA,NA] (similarity = 0.0)\n";
        else cout << "Best range = [" << res->leftVal << "," << res->rightVal << "] (similarity = " << res->similarity << ")\n";
        cout << "Query executed in " << chrono::duration_cast<chrono::milliseconds>(q1-q0).count() << " ms\n";
    }
    return 0;
}
