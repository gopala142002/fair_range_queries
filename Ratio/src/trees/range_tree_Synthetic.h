#ifndef RANGE_TREE_SYNTHETIC_EDITED_H
#define RANGE_TREE_SYNTHETIC_EDITED_H

#include <utility>
#include <vector>

struct Point {
  std::vector<double> coords;
  int original_index;

  Point(double cr, double cb, int index) : original_index(index) {
    coords.push_back(cr);
    coords.push_back(cb);
    coords.push_back(static_cast<double>(index));
  }
};

struct RangeTreeNode {
  int level;
  double split_val;
  int subtree_size;
  RangeTreeNode *left = nullptr;
  RangeTreeNode *right = nullptr;
  RangeTreeNode *next_tree = nullptr;
  Point *curr_point = nullptr;
  Point *min_point = nullptr;

  RangeTreeNode(int l, double sv, int ss, RangeTreeNode *lt, RangeTreeNode *rt,
                RangeTreeNode *nt, Point *cp, Point *mp)
      : level(l), split_val(sv), subtree_size(ss), left(lt), right(rt),
        next_tree(nt), curr_point(cp), min_point(mp) {}
};

struct QueryBox {
  std::vector<std::pair<double, double>> bounds;
};

// Global instrumentation
extern long long search_nodes_visited;
extern long long search_points_considered;

// Functions
void delete_tree(RangeTreeNode *node);
bool min_all(Point *p1, Point *p2);
bool inRangeQ(Point *p, const QueryBox &Q, int dims_to_check);
RangeTreeNode *build_tree(std::vector<Point *> &pts, int level,
                          int total_levels);

void find_most_similar_fair_j(RangeTreeNode *node, const QueryBox &Q,
                              int query_i, int query_j, int current_i,
                              int level, int total_levels,
                              double &max_similarity, int &best_j);

#endif
