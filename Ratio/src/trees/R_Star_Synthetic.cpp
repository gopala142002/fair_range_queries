#include "R_Star_Synthetic.h"
#include "../common/fair_range.h"
#include <iostream>
#include <algorithm>
#include <cmath>

const int MAX_ENTRIES = 16;
const int MIN_ENTRIES = 6;
const int totalDims = 2; // For Ratio project, it's always 2 (cr, cb)

double computeArea_rstar(const std::vector<double> &lo, const std::vector<double> &hi) {
    double area = 1.0;
    for (int d = 0; d < totalDims; ++d) {
        double ext = hi[d] - lo[d];
        if (ext <= 0) return 0.0;
        area *= ext;
    }
    return area;
}

double computeOverlap_rstar(const std::vector<double> &lo1, const std::vector<double> &hi1,
                            const std::vector<double> &lo2, const std::vector<double> &hi2) {
    double overlap = 1.0;
    for (int d = 0; d < totalDims; ++d) {
        double inter = std::min(hi1[d], hi2[d]) - std::max(lo1[d], lo2[d]);
        if (inter <= 0) return 0.0;
        overlap *= inter;
    }
    return overlap;
}

// These R* split axis/index functions are ported from the provided snippet,
// though the user's build algorithm uses purely alternating STR block loading.
int chooseSplitAxis_rstar(std::vector<Point *> &entries) {
    int total = (int)entries.size();
    int bestAxis = 0;
    double bestMarginSum = std::numeric_limits<double>::infinity();

    for (int axis = 0; axis < totalDims; ++axis) {
        std::sort(entries.begin(), entries.end(), [&](Point *a, Point *b) {
            return a->coords[axis] < b->coords[axis];
        });
        double marginSum = 0;
        for (int k = 0; k <= total - 2 * MIN_ENTRIES; ++k) {
            int split = MIN_ENTRIES + k;
            std::vector<double> lo1(totalDims, std::numeric_limits<double>::infinity());
            std::vector<double> hi1(totalDims, -std::numeric_limits<double>::infinity());
            std::vector<double> lo2(totalDims, std::numeric_limits<double>::infinity());
            std::vector<double> hi2(totalDims, -std::numeric_limits<double>::infinity());
            
            for (int i = 0; i < split; ++i) {
                Point *e = entries[i];
                for (int d = 0; d < totalDims; ++d) {
                    lo1[d] = std::min(lo1[d], e->coords[d]);
                    hi1[d] = std::max(hi1[d], e->coords[d]);
                }
            }
            for (int i = split; i < total; ++i) {
                Point *e = entries[i];
                for (int d = 0; d < totalDims; ++d) {
                    lo2[d] = std::min(lo2[d], e->coords[d]);
                    hi2[d] = std::max(hi2[d], e->coords[d]);
                }
            }
            for (int d = 0; d < totalDims; ++d) {
                marginSum += (hi1[d] - lo1[d]) + (hi2[d] - lo2[d]);
            }
        }
        if (marginSum < bestMarginSum) { 
            bestMarginSum = marginSum; 
            bestAxis = axis; 
        }
    }
    return bestAxis;
}

int chooseSplitIndex_rstar(std::vector<Point *> &entries, int axis) {
    int total = (int)entries.size();
    std::sort(entries.begin(), entries.end(), [&](Point *a, Point *b) {
        return a->coords[axis] < b->coords[axis];
    });
    int bestK = 0;
    double bestOverlap = std::numeric_limits<double>::infinity();
    double bestArea = std::numeric_limits<double>::infinity();
    
    for (int k = 0; k <= total - 2 * MIN_ENTRIES; ++k) {
        int split = MIN_ENTRIES + k;
        std::vector<double> lo1(totalDims, std::numeric_limits<double>::infinity());
        std::vector<double> hi1(totalDims, -std::numeric_limits<double>::infinity());
        std::vector<double> lo2(totalDims, std::numeric_limits<double>::infinity());
        std::vector<double> hi2(totalDims, -std::numeric_limits<double>::infinity());
        
        for (int i = 0; i < split; ++i) {
            Point *e = entries[i];
            for (int d = 0; d < totalDims; ++d) {
                lo1[d] = std::min(lo1[d], e->coords[d]);
                hi1[d] = std::max(hi1[d], e->coords[d]);
            }
        }
        for (int i = split; i < total; ++i) {
            Point *e = entries[i];
            for (int d = 0; d < totalDims; ++d) {
                lo2[d] = std::min(lo2[d], e->coords[d]);
                hi2[d] = std::max(hi2[d], e->coords[d]);
            }
        }
        double overlap = computeOverlap_rstar(lo1, hi1, lo2, hi2);
        double area = computeArea_rstar(lo1, hi1) + computeArea_rstar(lo2, hi2);
        
        if (overlap < bestOverlap || (overlap == bestOverlap && area < bestArea)) {
            bestOverlap = overlap;
            bestArea = area;
            bestK = k;
        }
    }
    return bestK;
}

void computeMBR_rstar(RStarTreeNode *node) {
    node->min_cr = std::numeric_limits<double>::infinity();
    node->max_cr = -std::numeric_limits<double>::infinity();
    node->min_cb = std::numeric_limits<double>::infinity();
    node->max_cb = -std::numeric_limits<double>::infinity();

    if (node->isLeaf) {
        for (Point *p : node->points) {
            node->min_cr = std::min(node->min_cr, p->coords[0]);
            node->max_cr = std::max(node->max_cr, p->coords[0]);
            node->min_cb = std::min(node->min_cb, p->coords[1]);
            node->max_cb = std::max(node->max_cb, p->coords[1]);
        }
    } else {
        for (RStarTreeNode *c : node->children) {
            node->min_cr = std::min(node->min_cr, c->min_cr);
            node->max_cr = std::max(node->max_cr, c->max_cr);
            node->min_cb = std::min(node->min_cb, c->min_cb);
            node->max_cb = std::max(node->max_cb, c->max_cb);
        }
    }
}

RStarTreeNode *build_rstar_tree_recursive(std::vector<Point *> &pts, int depth) {
    if (pts.empty()) return nullptr;

    RStarTreeNode *node = new RStarTreeNode();

    if ((int)pts.size() <= MAX_ENTRIES) {
        node->isLeaf = true;
        node->points = pts;
        computeMBR_rstar(node);
        return node;
    }

    // Sort by current dimension (cycle through dims for better packing)
    int dim = depth % totalDims;
    std::sort(pts.begin(), pts.end(), [&](Point *a, Point *b) {
        return a->coords[dim] < b->coords[dim];
    });

    node->isLeaf = false;

    int total = (int)pts.size();
    int groupSz = std::max(MAX_ENTRIES, (int)std::ceil((double)total / MAX_ENTRIES));

    for (int start = 0; start < total; start += groupSz) {
        int end = std::min(start + groupSz, total);
        std::vector<Point *> chunk(pts.begin() + start, pts.begin() + end);
        RStarTreeNode *child = build_rstar_tree_recursive(chunk, depth + 1);
        if (child) {
            node->children.push_back(child);
        }
    }

    computeMBR_rstar(node);
    return node;
}

RStarTreeNode *build_rstar_tree(std::vector<Point *> pts) {
    return build_rstar_tree_recursive(pts, 0);
}

void delete_rstar_tree(RStarTreeNode *node) {
    if (!node) return;
    for (RStarTreeNode *child : node->children) {
        delete_rstar_tree(child);
    }
    delete node;
}

void find_most_similar_fair_j_rstar_tree(RStarTreeNode *node, const QueryBox &Q,
                                         int query_i, int query_j, int current_i,
                                         double &max_similarity, int &best_j) {
    if (!node) return;

    search_nodes_visited++;

    if (node->min_cr > Q.bounds[0].second || node->max_cr < Q.bounds[0].first) return;
    if (node->min_cb > Q.bounds[1].second || node->max_cb < Q.bounds[1].first) return;

    if (node->isLeaf) {
        for (Point *point : node->points) {
            if (point->coords[0] < Q.bounds[0].first || point->coords[0] > Q.bounds[0].second ||
                point->coords[1] < Q.bounds[1].first || point->coords[1] > Q.bounds[1].second) {
                continue;
            }

            search_points_considered++;
            int j = point->original_index;
            if (j >= current_i) {
                double sim = calculate_tversky_index(query_i, query_j, current_i, j);
                if (sim > max_similarity) {
                    max_similarity = sim;
                    best_j = j;
                }
            }
        }
        return;
    }

    for (RStarTreeNode *child : node->children) {
        find_most_similar_fair_j_rstar_tree(child, Q, query_i, query_j, current_i,
                                            max_similarity, best_j);
    }
}
