#include "common.h"

Data readCSV(const string &filename) {
    ifstream file(filename);
    if (!file) throw runtime_error("Cannot open file: " + filename);

    string header; getline(file, header);
    auto cols = splitCSV(header);

    int indexCol = -1;
    vector<int> pairCols; vector<string> pairNames; set<string> colors;

    for (int i = 0; i < (int)cols.size(); ++i) {
        string name = cols[i];
        name.erase(0, name.find_first_not_of(" \t\r\n"));
        name.erase(name.find_last_not_of(" \t\r\n") + 1);
        if (name == "index" || name == "Index") indexCol = i;
        else if (isPairName(name)) {
            pairCols.push_back(i); pairNames.push_back(name);
            auto dash = name.find('-');
            colors.insert(name.substr(0, dash));
            colors.insert(name.substr(dash + 1));
        }
    }

    if (pairCols.empty()) { Data d; d.n=0; d.m=0; return d; }

    vector<vector<double>> rows; vector<double> key; string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        auto parts = splitCSV(line);
        double idxVal;
        if (indexCol >= 0 && indexCol < (int)parts.size())
            idxVal = parseDoubleSafe(parts[indexCol]);
        else { auto m2 = tryParse(parts.empty()?"":parts[0]); idxVal = m2 ? *m2 : (double)(key.size()+1); }
        key.push_back(idxVal);
        vector<double> vals(pairCols.size());
        for (int k = 0; k < (int)pairCols.size(); ++k) {
            int cIdx = pairCols[k];
            vals[k] = (cIdx < (int)parts.size() && !parts[cIdx].empty()) ? parseDoubleSafe(parts[cIdx]) : 0.0;
        }
        rows.push_back(vals);
    }

    int n=(int)rows.size(), m=(int)pairCols.size();
    Data data; data.n=n; data.m=m;
    data.keys=move(key); data.pairNames=move(pairNames); data.uniqueColors=move(colors);

    data.prefix.assign(m, vector<int>(n+1, 0));
    for (int t=0; t<n; ++t)
        for (int d=0; d<m; ++d)
            data.prefix[d][t+1] = (int)round(rows[t][d]);

    data.points.assign(n+1, vector<double>(m+1));
    for (int t=0; t<=n; ++t) {
        for (int d=0; d<m; ++d)
            data.points[t][d] = (t==0) ? 0.0 : (double)data.prefix[d][t];
        data.points[t][m] = (double)t;
    }
    return data;
}
