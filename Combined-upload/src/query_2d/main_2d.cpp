#include "data_2d.h"

#include "brute_2d.h"
#include "bfs_fair_2d.h"
#include "kd_tree_opt_2d.h"

#include "brute_fair_cov_2d.h"
#include "bfs_fair_cov_2d.h"
#include "kd_tree_opt_cov_2d.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Main driver for loading the dataset, reading fairness constraints,
// accepting queries, and running the selected 2D algorithm.
int main(int argc, char *argv[])
{

    // Expected arguments:
    // input file, number of colors, selected X/Y dimensions, and algorithm mode.
    if (argc < 6)
    {
        cout<< "Usage:\n"<< "main_2d <input> <num_colors> "<< "<dimX> <dimY> <mode>\n\n";
        cout<< "Modes:\n"<< "  brute\n"<< "  bfs\n"<< "  kd_opt\n"<< "  brute_cov\n"<< "  bfs_cov\n"<< "  kd_opt_cov\n";
        return 1;
    }

    // Read command-line arguments.
    string dataset = argv[1];

    int numColors =stoi(argv[2]);

    int dimX =stoi(argv[3]);

    int dimY =stoi(argv[4]);

    string mode =argv[5];

    // This driver is configured for datasets containing exactly three colors.
    if (numColors != 3)
    {
        cerr<< "This 2D program expects exactly 3 colors.\n"<< "Run with num_colors = 3.\n";
        return 1;
    }

    // Check whether the requested algorithm mode is supported.
    if (mode != "brute" &&mode != "bfs" &&mode != "kd_opt" &&mode != "brute_cov" &&mode != "bfs_cov" &&mode != "kd_opt_cov")
    {
        cout<< "Unknown mode: "<< mode<< "\n";

        cout<< "Available modes:\n"<< "brute\n"<< "bfs\n"<< "kd_opt\n"<< "brute_cov\n"<< "bfs_cov\n"<< "kd_opt_cov\n";
        return 1;
    }

    // Coverage modes require one minimum-count requirement for each color.
    bool coverageMode =(mode == "brute_cov"||mode == "bfs_cov" ||mode == "kd_opt_cov");

    // Load fairness constraints and start a query session for each configuration.
    while (loadConstraintsFromConsole())
    {
        // Start timing dataset loading and preprocessing.
        auto buildStart =chrono::high_resolution_clock::now();

        // Load the selected two dimensions and color labels from the dataset.
        RawData2D raw =
        readLiveData2D(dataset,dimX,dimY
        );

        // Stop if no valid points were loaded.
        if (raw.pts.empty())
        {
            cerr<< "No points loaded from dataset.\n";
            return 1;
        }

        // Verify that the loaded dataset actually contains three colors.
        if (raw.uniqueColorsList.size() != 3)
        {
            cerr<< "Expected exactly 3 colors, but dataset has "<< raw.uniqueColorsList.size()<< " colors.\n";
            return 1;
        }

        // Stop dataset loading timer.
        auto buildEnd =
        chrono::high_resolution_clock::now();

        cout<< "2D "<< mode<< " mode. Loaded in "<< fixed<< setprecision(4)<< chrono::duration<double, milli>(buildEnd - buildStart).count()<< " ms\n";

        cout<< "n="<< raw.pts.size()<< ", dimX="<< dimX<< ", dimY="<< dimY<< "\n";

        // Show the color order used by the coverage requirement vector.
        if (coverageMode)
        {
            cout<< "Coverage color order: "<< raw.uniqueColorsList[0]<< " "<< raw.uniqueColorsList[1]<< " "<< raw.uniqueColorsList[2]<< "\n";
        }

        string line;

        // Repeatedly accept rectangle queries until the user exits.
        while (true)
        {
            // Coverage algorithms additionally require three minimum color counts.
            if (coverageMode)
            {
                cout<< "Enter x_min x_max y_min y_max "<< "alpha beta "<< "req_color1 req_color2 req_color3 "<< "(-1 to exit):\n";
            }
            else
            {
                cout<< "Enter x_min x_max y_min y_max "<< "alpha beta (-1 to exit):\n";
            }

            // Read the complete query line.
            if (!getline(cin, line))
                break;

            // Find the first non-whitespace character.
            size_t first =line.find_first_not_of(" \t\r\n");

            if (first == string::npos)
                continue;

            line.erase(0,first);

            // Find the last non-whitespace character.
            size_t last =line.find_last_not_of(" \t\r\n");

            if (last != string::npos)
            {
                line.erase(last + 1);
            }

            // Exit the query loop when -1 is entered.
            if (line == "-1")
                break;

            // Parse query values from the input line.
            istringstream iss(line);

            double qX_min;
            double qX_max;

            double qY_min;
            double qY_max;

            double alpha;
            double beta;

            // Parse rectangle boundaries and Tversky parameters.
            if (!(iss>> qX_min>> qX_max>> qY_min>> qY_max>> alpha>> beta))
            {
                cerr<< "Invalid query format.\n";
                continue;
            }

            if (qX_min == -1)
                break;

            vector<int> required;

            if (coverageMode)
            {
                int req1;
                int req2;
                int req3;

                if (!(iss>> req1>> req2>> req3))
                {
                    cerr<< "Invalid coverage requirements. "<< "Expected exactly 3 integers: "<< "req_color1 req_color2 req_color3\n";
                        continue;
                }

                if (req1 < 0 ||req2 < 0 ||req3 < 0)
                {
                    cerr<< "Coverage requirements must be "<< "non-negative.\n";
                    continue;
                }

                required = {req1,req2,req3};
            }

            // Start query execution timer.
            auto queryStart =
            chrono::high_resolution_clock::now();

            // Stores the result returned by the selected algorithm.
            optional<Result2D> result;

            // Run exhaustive fair-rectangle search.
            if (mode == "brute")
            {
                result =brute_force_2d(raw,qX_min,qX_max,qY_min,qY_max,alpha,beta);
            }

            // Run priority-based BFS fair-rectangle search.
            else if (mode == "bfs")
            {
                result =bfsFair2D(raw,qX_min,qX_max,qY_min,qY_max,alpha,beta);
            }

            // Run optimized KD-tree fair-rectangle search.
            else if (mode == "kd_opt")
            {
                result =queryBestKD_opt_2d(raw,qX_min,qX_max,qY_min,qY_max,alpha,beta);
            }

            // Run exhaustive search with fairness and coverage constraints.
            else if (mode == "brute_cov")
            {
                result =brute_force_cov_2d(raw,qX_min,qX_max,qY_min,qY_max,alpha,beta,required);
            }

            // Run BFS search with fairness and coverage constraints.
            else if (mode == "bfs_cov")
            {
                result =bfsFairCov2D(raw,qX_min,qX_max,qY_min,qY_max,alpha,beta,required);
            }

            // Run optimized KD-tree search with fairness and coverage constraints.
            else if (mode == "kd_opt_cov")
            {
                result =queryBestKD_opt_cov_2d(raw,qX_min,qX_max,qY_min,qY_max,alpha,beta,required);
            }

            // Stop query execution timer.
            auto queryEnd =
            chrono::high_resolution_clock::now();
            // Print either an empty result or the best rectangle found.
            if (!result)
            {
                cout<< "Best rectangle = "<< "[NA,NA] x [NA,NA] "<< "(similarity = 0.0)\n";
            }
            else
            {
                cout<< "Best rectangle = ["<< result->x_min<< ","<< result->x_max<< "] x ["<< result->y_min<< ","<< result->y_max<< "] (similarity = "<< result->similarity<< ")\n";
            }

            cout<< "Query executed in "<< fixed<< setprecision(4)<< chrono::duration<double, milli>(queryEnd - queryStart).count()<< " ms\n";
        }
    }

    return 0;
}