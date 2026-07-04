#include "data_2d.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>

using namespace std;


// Fairness constraint configurations are defined elsewhere.
//
// diffPairs:
//     Stores difference-based fairness constraints.
//
// ratioPairs:
//     Stores ratio-based fairness constraints.
extern vector<DiffPairConfig> diffPairs;
extern vector<RatioPairConfig> ratioPairs;


// Reads a 2D dataset from the given file.
//
// dimX specifies which input coordinate is used as x.
// dimY specifies which input coordinate is used as y.
//
// Since each point contains two coordinates:
//
// dimX and dimY must be either 0 or 1.
//
// Example:
//
// Input point:
// 10.5 20.3 red
//
// If:
//
// dimX = 0
// dimY = 1
//
// then:
//
// p.x = 10.5
// p.y = 20.3
//
// The function also collects all unique colors appearing
// in the dataset.
RawData2D readLiveData2D(const string& filename,int dimX,int dimY) 
{
    // Object that will store all raw 2D data.
    RawData2D raw;


    // Open the input dataset file.
    ifstream file(filename);


    // Check whether the file was opened successfully.
    if (!file.is_open())
    {
        cerr << "Failed to open " << filename << endl;
        return raw;
    }


    // Validate the selected dimensions.
    //
    // Since the dataset is 2-dimensional,
    // valid coordinate indices are only 0 and 1.
    if (dimX < 0 || dimX >= 2 || dimY < 0 || dimY >= 2) 
    {
        cerr << "Invalid dimensions: "<< dimX << " " << dimY << endl;
        return raw;
    }


    // Used to store each line read from the file.
    string line;


    // Read the first line of the file.
    //
    // This line contains the number of points.
    if (!getline(file, line))
        return raw;


    // Convert the first line into an integer.
    int numPoints = stoi(line);


    // Read the next line.
    //
    // This appears to skip a header or metadata line
    // in the input file format.
    if (!getline(file, line))
        return raw;


    // Stores unique color labels.
    //
    // set automatically keeps only unique values
    // and stores them in sorted order.
    set<string> uniqueC;


    // Reserve enough memory for all expected points.
    raw.pts.reserve(numPoints);


    // Read every point from the dataset.
    for (int i = 0; i < numPoints; ++i) 
    { 
        // Try to read the next point line.
        if (!getline(file, line)) 
        {
            cerr << "Dataset ended early at point "<< i << endl;
            break;
        }


        // Convert the current line into a string stream
        // so individual values can be extracted.
        stringstream ss(line);


        // Two coordinates of the input point.
        double coord0;
        double coord1;


        // Color or category of the point.
        string color;


        // Expected input row format:
        //
        // coordinate_0 coordinate_1 color
        //
        // Example:
        //
        // 10.5 20.7 red
        if (!(ss >> coord0 >> coord1 >> color)) 
        {
            cerr << "Invalid row at point "<< i << ": "<< line << endl;
            continue;
        }


        // Store coordinates in an array so dimX and dimY
        // can select which coordinate becomes x and y.
        double coords[2] = {coord0,coord1};


        // Create one RawPoint object.
        RawPoint p;


        // Select x-coordinate according to dimX.
        p.x = coords[dimX];


        // Select y-coordinate according to dimY.
        p.y = coords[dimY];


        // Store the point's color.
        p.color = color;


        // Store the original position of this point
        // in the input dataset.
        p.original_index = i;


        // Add the point to the raw dataset.
        raw.pts.push_back(p);


        // Add the color to the set of unique colors.
        uniqueC.insert(color);
    }


    // Number of difference-based fairness constraints.
    raw.md = diffPairs.size();


    // Number of ratio-based fairness constraints.
    raw.mr = ratioPairs.size();


    // Convert the set of unique colors into a vector.
    //
    // Because uniqueC is a set, the colors will be
    // stored in sorted order.
    raw.uniqueColorsList.assign(
        uniqueC.begin(),
        uniqueC.end()
    );


    // Return the complete raw 2D dataset.
    return raw;
}


// Builds the Data structure for a selected subset of points.
//
// This function is mainly used after selecting an x-range
// and sorting the points in that x-range by y-coordinate.
//
// It builds:
//
// 1. Prefix counts for every color.
// 2. The y-coordinate keys.
// 3. Prefix-transformed fairness values.
//
// These structures allow fast fairness and coverage checks
// for intervals [YL, YR].
Data buildSubsetData(const vector<RawPoint>& subset,const RawData2D& raw)
{
    // Data structure that will be returned.
    Data data;


    // Number of points in the current subset.
    int n = subset.size();


    // Store subset size.
    data.n = n;


    // Number of difference constraints.
    data.md = raw.md;


    // Number of ratio constraints.
    data.mr = raw.mr;


    // Total number of transformed fairness dimensions.
    //
    // Every difference constraint contributes 1 dimension.
    //
    // Every ratio constraint contributes 2 dimensions:
    //
    // 1. Upper ratio condition.
    // 2. Lower ratio condition.
    data.m = data.md + 2 * data.mr;


    // Maps each color string to an integer index.
    //
    // Example:
    //
    // blue  -> 0
    // green -> 1
    // red   -> 2
    map<string, int> colorIndex;


    // Build the mapping from color name to color index.
    for (int i = 0;i < (int)raw.uniqueColorsList.size();++i) 
    {
        colorIndex[raw.uniqueColorsList[i]] = i;


        // Store the color in the Data structure.
        data.uniqueColors.insert(raw.uniqueColorsList[i]);
    }


    // Total number of unique colors.
    int numColors =raw.uniqueColorsList.size();


    // Allocate the color prefix-count table.
    //
    // Dimensions:
    //
    // numColors × (n + 1)
    //
    // data.prefix[c][t] =
    // number of points of color c among subset[0 ... t-1].
    data.prefix.assign(numColors,vector<int>(n + 1, 0));


    // keys[t] stores the y-coordinate of subset[t].
    data.keys.resize(n);


    // Build prefix counts for every color.
    for (int t = 0; t < n; ++t) 
    {
        // Store the y-coordinate of the current point.
        data.keys[t] = subset[t].y;


        // Copy previous prefix counts for every color.
        //
        // Before considering subset[t]:
        //
        // prefix[c][t + 1] = prefix[c][t]
        for (int c = 0; c < numColors; ++c) 
        {
            data.prefix[c][t + 1] =data.prefix[c][t];
        }


        // Find the color index of the current point.
        auto it =colorIndex.find(subset[t].color);


        // Increment the prefix count corresponding
        // to the current point's color.
        if (it != colorIndex.end()) 
        {
            data.prefix[it->second][t + 1]++;
        }
    }


    // Allocate transformed prefix-point representation.
    //
    // There are n + 1 prefix states:
    //
    // prefix 0
    // prefix 1
    // ...
    // prefix n
    //
    // Each prefix state stores:
    //
    // data.m fairness-related dimensions
    //
    // plus one additional value containing t.
    data.points.assign(n + 1,vector<double>(data.m + 1, 0.0));


    // Build transformed fairness values
    // for every prefix position t.
    for (int t = 0; t <= n; ++t) 
    {
        // Current transformed dimension index.
        int dim = 0;


        // Process every difference-based fairness constraint.
        for (const auto& config : diffPairs) 
        {
            // Find the prefix-array index of colorA.
            //
            // If the color does not exist, use -1.
            int idxA =colorIndex.count(config.colorA)? colorIndex[config.colorA]: -1;


            // Find the prefix-array index of colorB.
            int idxB =colorIndex.count(config.colorB)? colorIndex[config.colorB]: -1;


            // Number of colorA points in prefix [0, t).
            double countA =(idxA != -1)? data.prefix[idxA][t]: 0;


            // Number of colorB points in prefix [0, t).
            double countB =(idxB != -1)? data.prefix[idxB][t]: 0;


            // Store the transformed prefix value:
            //
            // weightA * countA - weightB * countB
            //
            // For an interval [YL, YR], subtracting two prefix
            // transformed values gives the corresponding weighted
            // count difference for that interval.
            data.points[t][dim++] =config.weightA * countA-config.weightB * countB;
        }


        // Process every ratio-based fairness constraint.
        //
        // Each ratio constraint creates two transformed dimensions.
        for (const auto& config : ratioPairs) 
        {
            // Find the index of colorA.
            int idxA =colorIndex.count(config.colorA)? colorIndex[config.colorA]: -1;


            // Find the index of colorB.
            int idxB =colorIndex.count(config.colorB)? colorIndex[config.colorB]: -1;


            // Prefix count of colorA.
            double countA =(idxA != -1)? data.prefix[idxA][t]: 0;


            // Prefix count of colorB.
            double countB =(idxB != -1)? data.prefix[idxB][t]: 0;


            // First transformed ratio expression.
            //
            // This represents the upper ratio-bound condition.
            double cr =config.weightA * countA-config.weightB* countB* (1.0 + config.delta);


            // Second transformed ratio expression.
            //
            // This represents the lower ratio-bound condition.
            double cb =config.weightB* countB* (1.0 - config.delta)-config.weightA * countA;


            // Store upper-bound transformed value.
            data.points[t][dim++] = cr;


            // Store lower-bound transformed value.
            data.points[t][dim++] = cb;
        }

        // Store the prefix index t in the last dimension.
        //
        // data.points[t] therefore contains:
        //
        // [fairness dimension 0,
        //  fairness dimension 1,
        //  ...
        //  fairness dimension m-1,
        //  t]
        data.points[t][dim] =
            (double)t;
    }


    // Return the constructed subset data structure.
    return data;
}