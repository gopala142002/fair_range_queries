#include "common.h"
#include "config.h"

// Define the global config vectors here
vector<DiffPairConfig> diffPairs;
vector<RatioPairConfig> ratioPairs;

bool loadConstraintsFromConsole() {
    diffPairs.clear();
    ratioPairs.clear();
    
    int nd;
    cout << "Enter number of Difference constraints you want to add (-1 to exit program): ";
    if (!(cin >> nd) || nd == -1) return false;
    
    for (int i = 0; i < nd; ++i) {
        string cA, cB;
        double wA, wB, eps;
        cout << "  Constraint " << i+1 << " - Enter ColorA ColorB weightA weightB Epsilon (e.g., Red Blue 1.0 1.0 2.0): ";
        cin >> cA >> cB >> wA >> wB >> eps;
        diffPairs.push_back({cA, cB, wA, wB, eps});
    }
    
    int nr;
    cout << "Enter number of Ratio constraints you want to add: ";
    if (!(cin >> nr)) return false;
    
    for (int i = 0; i < nr; ++i) {
        string cA, cB;
        double wA, wB, delta;
        cout << "  Constraint " << i+1 << " - Enter ColorA ColorB weightA weightB delta (e.g., Red Blue 1.0 1.0 0.2): ";
        cin >> cA >> cB >> wA >> wB >> delta;
        ratioPairs.push_back({cA, cB, wA, wB, delta});
    }
    
    cout << "Loaded " << diffPairs.size() << " diff constraints and " 
         << ratioPairs.size() << " ratio constraints.\n";
    return true;
}

Data readCSV(const string &filename) {
  auto tRead0 = chrono::high_resolution_clock::now();

  ifstream file(filename);
  if (!file)
    throw runtime_error("Cannot open file: " + filename);

  string header;
  getline(file, header);
  auto cols = splitCSV(header);

  int indexCol = -1;
  int colorCol = -1;

  for (int i = 0; i < (int)cols.size(); ++i) {
    string name = cols[i];
    name.erase(0, name.find_first_not_of(" \t\r\n"));
    name.erase(name.find_last_not_of(" \t\r\n") + 1);
    if (name == "index" || name == "Index")
      indexCol = i;
    if (name == "color" || name == "Color")
      colorCol = i;
  }

  if (colorCol == -1)
    throw runtime_error("No 'color' column found in file.");

  vector<double> key;
  vector<string> rowColors;
  string line;

  int rowCount = 0;
  while (getline(file, line)) {
    if (line.empty())
      continue;
    // if (rowCount >= 10000) break;
    rowCount++;
    auto parts = splitCSV(line);
    double idxVal;
    if (indexCol >= 0 && indexCol < (int)parts.size())
      idxVal = parseDoubleSafe(parts[indexCol]);
    else {
      auto m2 = tryParse(parts.empty() ? "" : parts[0]);
      idxVal = m2 ? *m2 : (double)(key.size() + 1);
    }
    key.push_back(idxVal);

    string c = (colorCol < (int)parts.size()) ? parts[colorCol] : "";
    c.erase(0, c.find_first_not_of(" \t\r\n"));
    c.erase(c.find_last_not_of(" \t\r\n") + 1);
    rowColors.push_back(c);
  }

  int n = (int)key.size();
  Data data;
  data.n = n;
  data.md = diffPairs.size();
  data.mr = ratioPairs.size();
  data.m = data.md + 2 * data.mr;
  data.keys = move(key);

  // Collect unique colors
  for (const auto &c : rowColors) {
    data.uniqueColors.insert(c);
  }

  // Assign indices to colors
  vector<string> colorNames(data.uniqueColors.begin(), data.uniqueColors.end());
  map<string, int> colorIndex;
  for (int i = 0; i < (int)colorNames.size(); ++i) {
    colorIndex[colorNames[i]] = i;
  }

  int numColors = colorNames.size();

  auto tParse = chrono::high_resolution_clock::now();

  // Compute basic prefix sums (count of each color up to row t)
  data.prefix.assign(numColors, vector<int>(n + 1, 0));
  for (int t = 0; t < n; ++t) {
    for (int c = 0; c < numColors; ++c) {
      data.prefix[c][t + 1] = data.prefix[c][t];
    }
    auto it = colorIndex.find(rowColors[t]);
    if (it != colorIndex.end()) {
      data.prefix[it->second][t + 1]++;
    }
  }

  // Now build points array. Dimensions: [md diffs ..., 2*mr ratio dims ...,
  // index]
  data.points.assign(n + 1, vector<double>(data.m + 1, 0.0));

  for (int t = 0; t <= n; ++t) {
    int dim = 0;

    // 1. Difference dimensions
    for (const auto &config : diffPairs) {
      int idxA =
          colorIndex.count(config.colorA) ? colorIndex[config.colorA] : -1;
      int idxB =
          colorIndex.count(config.colorB) ? colorIndex[config.colorB] : -1;

      double countA = (idxA != -1) ? data.prefix[idxA][t] : 0;
      double countB = (idxB != -1) ? data.prefix[idxB][t] : 0;

      data.points[t][dim++] = config.weightA * countA - config.weightB * countB;
    }

    // 2. Ratio dimensions
    for (const auto &config : ratioPairs) {
      int idxA =
          colorIndex.count(config.colorA) ? colorIndex[config.colorA] : -1;
      int idxB =
          colorIndex.count(config.colorB) ? colorIndex[config.colorB] : -1;

      double countA = (idxA != -1) ? data.prefix[idxA][t] : 0;
      double countB = (idxB != -1) ? data.prefix[idxB][t] : 0;

      double cr = config.weightA * countA -
                  config.weightB * countB * (1.0 + config.delta);
      double cb = config.weightB * countB * (1.0 - config.delta) -
                  config.weightA * countA;

      data.points[t][dim++] = cr;
      data.points[t][dim++] = cb;
    }

    // 3. Index dimension
    data.points[t][dim] = (double)t;
  }
  auto tCumul = chrono::high_resolution_clock::now();
  data.pointsBuildTimeMs = chrono::duration<double, std::milli>(tCumul - tParse).count();

  cout << "File loaded in " << fixed << setprecision(4) 
       << chrono::duration<double, std::milli>(tParse - tRead0).count() << " ms\n";

  return data;
}

void setQueryBounds(const Data &data, const vector<double> &center,
                    vector<double> &lo, vector<double> &hi, bool isRightSide) {
  for (int d = 0; d < data.md; ++d) {
    lo[d] = center[d] - diffPairs[d].epsilon;
    hi[d] = center[d] + diffPairs[d].epsilon;
  }
  for (int r = 0; r < data.mr; ++r) {
    int d1 = data.md + 2 * r;
    int d2 = data.md + 2 * r + 1;
    if (isRightSide) {
      lo[d1] = center[d1];
      hi[d1] = numeric_limits<double>::infinity();
      lo[d2] = center[d2];
      hi[d2] = numeric_limits<double>::infinity();
    } else {
      lo[d1] = -numeric_limits<double>::infinity();
      hi[d1] = center[d1];
      lo[d2] = -numeric_limits<double>::infinity();
      hi[d2] = center[d2];
    }
  }
}

bool isFair(const Data &data, int L, int R) {
  if (L < 1 || R > data.n || L > R)
    return false;
  for (int d = 0; d < data.md; ++d) {
    double diff = data.points[R][d] - data.points[L - 1][d];
    if (abs(diff) > diffPairs[d].epsilon)
      return false;
  }
  for (int r = 0; r < data.mr; ++r) {
    int d1 = data.md + 2 * r;
    int d2 = data.md + 2 * r + 1;
    if (data.points[R][d1] - data.points[L - 1][d1] > 0.0)
      return false;
    if (data.points[R][d2] - data.points[L - 1][d2] > 0.0)
      return false;
  }
  return true;
}
