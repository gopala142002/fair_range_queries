#include "coverage.h"

CoverageData buildCoverageData(const string &filename,
                                const vector<string> &colorOrder) {
    ifstream file(filename);
    if (!file) throw runtime_error("Cannot open file: " + filename);

    string header; getline(file, header);
    auto cols = splitCSV(header);

    // Find index and color column positions
    int colorCol=-1;
    for (int i=0; i<(int)cols.size(); ++i) {
        string name = cols[i];
        name.erase(0, name.find_first_not_of(" \t\r\n"));
        name.erase(name.find_last_not_of(" \t\r\n")+1);
        
        if (name=="color" || name=="Color") colorCol=i;
    }

    if (colorCol==-1) throw runtime_error("No 'color' column found in file.");

    // Read all rows and collect color per row
    vector<string> rowColors;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        auto parts = splitCSV(line);
        string c = (colorCol < (int)parts.size()) ? parts[colorCol] : "";
        c.erase(0, c.find_first_not_of(" \t\r\n"));
        c.erase(c.find_last_not_of(" \t\r\n")+1);
        rowColors.push_back(c);
    }

    int n = (int)rowColors.size();

    // Use provided color order or derive from data
    CoverageData cov;
    if (!colorOrder.empty()) {
        cov.colorNames = colorOrder;
    } else {
        // Collect unique colors preserving first-seen order
        set<string> seen;
        for (auto &c : rowColors)
            if (seen.insert(c).second) cov.colorNames.push_back(c);
    }

    int numColors = (int)cov.colorNames.size();
    for (int i=0; i<numColors; ++i)
        cov.colorIndex[cov.colorNames[i]] = i;

    // Build prefix sum per color: colorPrefix[c][t] = count of color c in rows 0..t-1
    cov.colorPrefix.assign(numColors, vector<int>(n+1, 0));
    for (int t=0; t<n; ++t) {
        for (int c=0; c<numColors; ++c)
            cov.colorPrefix[c][t+1] = cov.colorPrefix[c][t];
        auto it = cov.colorIndex.find(rowColors[t]);
        if (it != cov.colorIndex.end())
            cov.colorPrefix[it->second][t+1]++;
    }

    return cov;
}

int findCoveragePoint(const CoverageData &cov, int p,
                      const vector<int> &required, int n) {
    int coveragePoint = p+1; // minimum valid R starts just after p

    for (int c=0; c<(int)cov.colorNames.size(); ++c) {
        if (required[c] <= 0) continue;

        // Need colorPrefix[c][R] - colorPrefix[c][p] >= required[c]
        // i.e. colorPrefix[c][R] >= colorPrefix[c][p] + required[c]
        int target = cov.colorPrefix[c][p] + required[c];
        const auto &pref = cov.colorPrefix[c];

        // Binary search for minimum R in [p+1, n] where pref[R] >= target
        int lo=p+1, hi=n, found=-1;
        while (lo<=hi) {
            int mid=(lo+hi)/2;
            if (pref[mid] >= target) { found=mid; hi=mid-1; }
            else lo=mid+1;
        }

        if (found==-1) return -1; // coverage impossible for this color
        coveragePoint = max(coveragePoint, found);
    }

    return coveragePoint;
}

int findMaxLForCoverage(const CoverageData &cov, int r,
                        const vector<int> &required, int n) {
    int maxL_minus_1 = r - 1; // maximum valid L-1 is just before r

    for (int c=0; c<(int)cov.colorNames.size(); ++c) {
        if (required[c] <= 0) continue;

        // Need colorPrefix[c][r] - colorPrefix[c][L-1] >= required[c]
        // i.e. colorPrefix[c][L-1] <= colorPrefix[c][r] - required[c]
        int target = cov.colorPrefix[c][r] - required[c];
        if (target < 0) return -1; // impossible even if L-1=0

        const auto &pref = cov.colorPrefix[c];

        // Binary search for maximum L-1 in [0, r-1] where pref[L-1] <= target
        int lo=0, hi=r-1, found=-1;
        while (lo<=hi) {
            int mid=(lo+hi)/2;
            if (pref[mid] <= target) { found=mid; lo=mid+1; }
            else hi=mid-1;
        }

        if (found==-1) return -1;
        maxL_minus_1 = min(maxL_minus_1, found);
    }

    return maxL_minus_1;
}
