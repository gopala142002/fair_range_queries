// Converts existing dataset to coverage-ready format by adding per-color prefix sum columns.
#include <bits/stdc++.h>
using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Usage: convert_dataset <input_file> <output_file>\n";
        cout << "Example: convert_dataset rangetree_1000_3 rangetree_1000_3_cov\n";
        return 1;
    }

    string inputFile  = argv[1];
    string outputFile = argv[2];

    ifstream fin(inputFile);
    if (!fin) { cerr << "Cannot open input file: " << inputFile << "\n"; return 1; }

    // Read header
    string header; getline(fin, header);

    // Remove trailing \r if present (Windows line endings)
    if (!header.empty() && header.back()=='\r') header.pop_back();

    // Parse header to find color column
    vector<string> cols;
    stringstream hss(header);
    string tok;
    while (getline(hss, tok, ',')) {
        if (!tok.empty() && tok.back()=='\r') tok.pop_back();
        cols.push_back(tok);
    }

    int colorCol = -1;
    for (int i=0; i<(int)cols.size(); ++i) {
        string name = cols[i];
        if (name=="color" || name=="Color") { colorCol=i; break; }
    }
    if (colorCol==-1) { cerr << "No 'color' column found.\n"; return 1; }

    // Read all rows
    vector<string> rawLines;
    vector<string> colorPerRow;
    string line;
    while (getline(fin, line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty()) continue;
        rawLines.push_back(line);

        // Extract color value
        stringstream rss(line);
        string field; int col=0; string colorVal;
        while (getline(rss, field, ',')) {
            if (col==colorCol) { colorVal=field; break; }
            col++;
        }
        colorPerRow.push_back(colorVal);
    }
    fin.close();

    int n = (int)rawLines.size();

    // Collect unique colors preserving first-seen order
    vector<string> colorNames;
    map<string,int> colorIndex;
    for (auto &c : colorPerRow) {
        if (colorIndex.find(c)==colorIndex.end()) {
            colorIndex[c] = (int)colorNames.size();
            colorNames.push_back(c);
        }
    }

    int numColors = (int)colorNames.size();
    cout << "Found " << numColors << " colors: ";
    for (auto &c : colorNames) cout << c << " ";
    cout << "\n";

    // Build prefix sums per color
    // prefix[c][t] = count of color c in rows 0..t-1
    vector<vector<int>> prefix(numColors, vector<int>(n+1, 0));
    for (int t=0; t<n; ++t) {
        for (int c=0; c<numColors; ++c)
            prefix[c][t+1] = prefix[c][t];
        auto it = colorIndex.find(colorPerRow[t]);
        if (it!=colorIndex.end())
            prefix[it->second][t+1]++;
    }

    // Write output file
    ofstream fout(outputFile);
    if (!fout) { cerr << "Cannot open output file: " << outputFile << "\n"; return 1; }

    // Write new header: original columns + prefix_ColorName for each color
    fout << header;
    for (auto &c : colorNames) fout << ",prefix_" << c;
    fout << "\n";

    // Write each row with appended prefix sum values
    for (int t=0; t<n; ++t) {
        fout << rawLines[t];
        for (int c=0; c<numColors; ++c)
            fout << "," << prefix[c][t+1];
        fout << "\n";
    }

    fout.close();
    cout << "Converted: " << inputFile << " -> " << outputFile << "\n";
    cout << "Added columns: ";
    for (auto &c : colorNames) cout << "prefix_" << c << " ";
    cout << "\n";
    cout << "Rows: " << n << "\n";

    return 0;
}
