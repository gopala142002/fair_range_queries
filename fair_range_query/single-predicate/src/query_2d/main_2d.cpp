#include "data_2d.h"

#include "bfs_fair_2d.h"
#include "kd_tree_opt_2d.h"

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

int main(int argc, char *argv[])
{
    if (argc < 6)
    {
        cout << "Usage:\n"
             << "main_2d <input> <num_colors> <dimX> <dimY> <mode>\n\n";

        cout << "Modes:\n"
             << "  bfs\n"
             << "  kd_opt\n"
             << "  bfs_cov\n"
             << "  kd_opt_cov\n";

        return 1;
    }

    string dataset = argv[1];

    int numColors = stoi(argv[2]);
    int dimX = stoi(argv[3]);
    int dimY = stoi(argv[4]);

    string mode = argv[5];

    if (numColors != 3)
    {
        cerr << "This 2D program expects exactly 3 colors.\n"
             << "Run with num_colors = 3.\n";

        return 1;
    }

    if (mode != "bfs" &&
        mode != "kd_opt" &&
        mode != "bfs_cov" &&
        mode != "kd_opt_cov")
    {
        cout << "Unknown mode: " << mode << "\n";

        cout << "Available modes:\n"
             << "bfs\n"
             << "kd_opt\n"
             << "bfs_cov\n"
             << "kd_opt_cov\n";

        return 1;
    }

    bool coverageMode =
        mode == "bfs_cov" ||
        mode == "kd_opt_cov";

    while (loadConstraintsFromConsole())
    {
        auto buildStart =
            chrono::high_resolution_clock::now();

        RawData2D raw =
            readLiveData2D(
                dataset,
                dimX,
                dimY
            );

        if (raw.pts.empty())
        {
            cerr << "No points loaded from dataset.\n";
            return 1;
        }

        if (raw.uniqueColorsList.size() != 3)
        {
            cerr << "Expected exactly 3 colors, but dataset has "
                 << raw.uniqueColorsList.size()
                 << " colors.\n";

            return 1;
        }

        auto buildEnd =
            chrono::high_resolution_clock::now();

        cout << "2D "
             << mode
             << " mode. Loaded in "
             << fixed
             << setprecision(4)
             << chrono::duration<double, milli>(
                    buildEnd - buildStart
                ).count()
             << " ms\n";

        cout << "n="
             << raw.pts.size()
             << ", dimX="
             << dimX
             << ", dimY="
             << dimY
             << "\n";

        if (coverageMode)
        {
            cout << "Coverage color order: "
                 << raw.uniqueColorsList[0] << " "
                 << raw.uniqueColorsList[1] << " "
                 << raw.uniqueColorsList[2] << "\n";
        }

        string line;

        while (true)
        {
            if (coverageMode)
            {
                cout << "Enter x_min x_max y_min y_max "
                     << "alpha beta "
                     << "req_color1 req_color2 req_color3 "
                     << "(-1 to exit):\n";
            }
            else
            {
                cout << "Enter x_min x_max y_min y_max "
                     << "alpha beta (-1 to exit):\n";
            }

            if (!getline(cin, line))
                break;

            size_t first =
                line.find_first_not_of(" \t\r\n");

            if (first == string::npos)
                continue;

            line.erase(0, first);

            size_t last =
                line.find_last_not_of(" \t\r\n");

            if (last != string::npos)
            {
                line.erase(last + 1);
            }

            if (line == "-1")
                break;

            istringstream iss(line);

            double qX_min;
            double qX_max;

            double qY_min;
            double qY_max;

            double alpha;
            double beta;

            if (!(iss >> qX_min
                      >> qX_max
                      >> qY_min
                      >> qY_max
                      >> alpha
                      >> beta))
            {
                cerr << "Invalid query format.\n";
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

                if (!(iss >> req1
                          >> req2
                          >> req3))
                {
                    cerr << "Invalid coverage requirements. "
                         << "Expected exactly 3 integers: "
                         << "req_color1 req_color2 req_color3\n";

                    continue;
                }

                if (req1 < 0 ||
                    req2 < 0 ||
                    req3 < 0)
                {
                    cerr << "Coverage requirements must be "
                         << "non-negative.\n";

                    continue;
                }

                required = {
                    req1,
                    req2,
                    req3
                };
            }

            auto queryStart =
                chrono::high_resolution_clock::now();

            optional<Result2D> result;

            if (mode == "bfs")
            {
                result =
                    bfsFair2D(
                        raw,
                        qX_min,
                        qX_max,
                        qY_min,
                        qY_max,
                        alpha,
                        beta
                    );
            }
            else if (mode == "kd_opt")
            {
                result =
                    queryBestKD_opt_2d(
                        raw,
                        qX_min,
                        qX_max,
                        qY_min,
                        qY_max,
                        alpha,
                        beta
                    );
            }
            else if (mode == "bfs_cov")
            {
                result =
                    bfsFairCov2D(
                        raw,
                        qX_min,
                        qX_max,
                        qY_min,
                        qY_max,
                        alpha,
                        beta,
                        required
                    );
            }
            else if (mode == "kd_opt_cov")
            {
                result =
                    queryBestKD_opt_cov_2d(
                        raw,
                        qX_min,
                        qX_max,
                        qY_min,
                        qY_max,
                        alpha,
                        beta,
                        required
                    );
            }

            auto queryEnd =
                chrono::high_resolution_clock::now();

            if (!result)
            {
                cout << "Best rectangle = "
                     << "[NA,NA] x [NA,NA] "
                     << "(similarity = 0.0)\n";
            }
            else
            {
                cout << "Best rectangle = ["
                     << result->x_min
                     << ","
                     << result->x_max
                     << "] x ["
                     << result->y_min
                     << ","
                     << result->y_max
                     << "] (similarity = "
                     << result->similarity
                     << ")\n";
            }

            cout << "Query executed in "
                 << fixed
                 << setprecision(4)
                 << chrono::duration<double, milli>(
                        queryEnd - queryStart
                    ).count()
                 << " ms\n";
        }
    }

    return 0;
}