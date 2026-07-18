#include <iostream>
#include <vector>
#include <limits>
#include <stack>
#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <queue>
#include <bits/stdc++.h>
using namespace std;
#define DEBUG 0

#include "parse_constraints.hpp"

int d, t;
double eps = 1e-9;
double epsilon = 0;
double Wb=1.0, Wr=1.0;
int orgCr=0,orgCb=0;
int numColors=3;

std::vector<double> color_weights;
bool use_coverage = false;
std::vector<int> min_coverage;

std::vector<std::vector<PairwiseConstraint>> constraints_matrix;

bool isFeasible(const std::vector<int> &color_count) {
    if (use_coverage) {
        for (int i = 1; i < numColors; ++i) {
            if (color_count[i] < min_coverage[i]) return false;
        }
    }
    for (int i = 1; i < numColors; ++i) {
        for (int j = 1; j < numColors; ++j) {
            if (i == j) continue;
            const auto &c = constraints_matrix[i][j];
            if (c.type == 1) { // Ratio constraint C_i / C_j <= 1 + threshold
                double max_ratio = 1.0 + c.threshold;
                double val_i = c.weight_i * color_count[i];
                double val_j = c.weight_j * color_count[j];
                if (val_j == 0.0) {
                    if (val_i > 0.0) return false;
                } else {
                    if (val_i / val_j > max_ratio) return false;
                }
            } else if (c.type == 2 && i < j) { // Difference constraint
                double val_i = c.weight_i * color_count[i];
                double val_j = c.weight_j * color_count[j];
                if (abs(val_i - val_j) > c.threshold) {
                    return false;
                }
            }
        }
    }
    return true;
}
struct Point {
    vector<double> coords; // d + t dimensions combined
    int index;
    int color_id;
};

struct QueryBox {
    vector<pair<double, double>> bounds;
};

struct FeatureBox {
    vector<pair<double, double>> bounds;
};

struct RangeTreeNode {
    int level;
    double split_val;
    int subtree_size;
    vector<int>color_count;
    RangeTreeNode *left;
    RangeTreeNode *right;
    RangeTreeNode *next_tree;
    Point *min_point;
    Point *curr_point;

    RangeTreeNode(int l, double val, int subcnt, RangeTreeNode *le, RangeTreeNode *ri, RangeTreeNode *ne, Point *cp, Point *mp, vector<int> v)
        : level(l), split_val(val), subtree_size(subcnt), left(le), right(ri), next_tree(ne), curr_point(cp), min_point(mp), color_count(v) {}
};

vector<Point> points;

struct HashPair {
    template <class T1, class T2>
    size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        // Combine the hash values
        return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
    }
};

struct HashVectorOfPairs {
    size_t operator()(const std::vector<std::pair<double, double>>& v) const {
        size_t seed = v.size();
        for (const auto& p : v) {
            seed ^= (HashPair{}(p) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
        }
        return seed;
    }
};


bool inRangeBox(const Point &p, const QueryBox &box) {
    for (int i = 0; i < d; ++i) {
        if (p.coords[i] < box.bounds[i].first || p.coords[i] > box.bounds[i].second)
            return false;
    }
    return true;
}

bool inFeatureBox(const Point &p, const FeatureBox &box) {
    for (int i = 0; i < t; ++i) {
        if (p.coords[d + i] < box.bounds[i].first || p.coords[d + i] > box.bounds[i].second)
            return false;
    }
    return true;
}

bool min_all_masked(Point *p1, Point *p2, int mask) {
    for (int i = d; i >=0; i--) {
        int dim_idx = d + i;  // Feature dimension index
        
        double val1 = p1->coords[dim_idx];
        double val2 = p2->coords[dim_idx];
        
        // Flip comparison based on mask for this dimension
        if ((mask >> i) & 1 == 0) {
            // Original corner uses left bound - prefer smaller values (minimize)
            if (val1 < val2) return true;   // p1 dominates p2
            if (val1 > val2) return false;  // p2 dominates p1
        } else {
            // Original corner uses right bound - prefer larger values (maximize) 
            if (val1 > val2) return true;   // p1 dominates p2
            if (val1 < val2) return false;  // p2 dominates p1
        }
    }
    return false; // Equal in all dimensions
}

bool min_all(Point *p1, Point *p2) {
    for (int i = t - 1; i >= 0; i--) {
        if (p1->coords[d + i] < p2->coords[d + i]) return true;
        if (p1->coords[d + i] > p2->coords[d + i]) return false;
    }
    return false;
}

RangeTreeNode* build_tree(vector<Point *> &pts, int level, int last_dim, int total_levels) {
    if (pts.empty()) return nullptr;

    int dim = level >= last_dim ? level+1:level;
    if(level==total_levels-1)
        dim = last_dim;
    sort(pts.begin(), pts.end(), [&](Point *a, Point *b) {
        return a->coords[dim] < b->coords[dim];
    });
    int mid = pts.size() / 2;
    Point *curr = pts[mid];
    double split_val = curr->coords[dim];

    vector<Point *> left_pts(pts.begin(), pts.begin() + mid);
    vector<Point *> right_pts(pts.begin() + mid + 1, pts.end());

    RangeTreeNode *left = build_tree(left_pts, level, last_dim, total_levels);
    RangeTreeNode *right = build_tree(right_pts, level, last_dim, total_levels);
    RangeTreeNode *next = nullptr;
    if (level < total_levels - 1)
        next = build_tree(pts, level + 1, last_dim, total_levels);
    
    vector<int> v(numColors,0);
    if(left){
        for(int i=0;i<numColors;i++) v[i]+= left->color_count[i];
    }
    if(right){
        for(int i=0;i<numColors;i++) v[i]+= right->color_count[i];
    }
    v[curr->color_id]++;
    Point *min_p = curr;
    if (left && min_all(left->min_point, min_p)) min_p = left->min_point;
    if (right && min_all(right->min_point, min_p)) min_p = right->min_point;
    if (next && min_all(next->min_point, min_p)) min_p = next->min_point;

    return new RangeTreeNode(level, split_val, (int)(pts.size()), left, right, next, curr, min_p, v);
}

bool inRangeQ(Point* p, QueryBox & Q, int D, int T) {
    for (int i = 0; i < D; ++i) {
        if (p->coords[i] < Q.bounds[i].first || p->coords[i] > Q.bounds[i].second)
            return false;
    }
    return true;
}

bool inRangeQExpand(Point* p, QueryBox & Q, int D, int T, int last_dim) {
    for (int i = 0; i < D; ++i) {
        if(i==last_dim)continue;
        if (p->coords[i] < Q.bounds[i].first || p->coords[i] > Q.bounds[i].second)
            return false;
    }
    return true;
}

bool inRangeF(Point* p, FeatureBox & F, int D, int T) {
    for (int i = D; i < D+T; ++i) {
        if (p->coords[i] < F.bounds[i].first || p->coords[i] > F.bounds[i].second)
            return false;
    }
    return true;
}

RangeTreeNode* findSplitNode(RangeTreeNode* root, double low, double high) {
    while (root && (root->left || root->right) &&
           (high < root->split_val || low > root->split_val)) {
        if (high < root->split_val)
            root = root->left;
        else
            root = root->right;
    }
    return root;
}

void dDRangeQuery(RangeTreeNode* root,  QueryBox& Q, FeatureBox& F, int level, int total_levels, int last_dim, int d, int t, vector<Point*>& out, int &size, vector<int> &color_count) {
    if (!root || level >= total_levels) return;

    int dim;
    if(level<d){
        dim = level >= last_dim ? level+1:level;
        if(level==total_levels-1)
            dim = last_dim;
    }
    else dim=level-d;
    double low = (level < d) ? Q.bounds[dim].first : F.bounds[dim].first;
    double high = (level < d) ? Q.bounds[dim].second : F.bounds[dim].second;

    RangeTreeNode* split = findSplitNode(root, low, high);
    if (!split) return;

    if(inRangeQ(split->curr_point, Q, d, t)){
        out.push_back(split->curr_point);
        size++;
        color_count[split->curr_point->color_id]++;
    }
    // Traverse left path;
    RangeTreeNode* v = split->left;
    while (v) {
        if (low <= v->split_val) {
            if(inRangeQ(v->curr_point, Q, d, t)){
                out.push_back(v->curr_point);
                size++;
                color_count[v->curr_point->color_id]++;
            }
            if(level == total_levels-1){
                if(v->right) size+= v->right->subtree_size;
                if(v->right){
                    for(int i=0;i<numColors;i++) color_count[i]+= v->right->color_count[i];
                }
            }
            if(v->right && v->right->next_tree){
                dDRangeQuery(v->right->next_tree, Q, F, level + 1, total_levels, last_dim, d, t, out, size, color_count);
            }
            v = v->left;
        } else {
            v = v->right;
        }
    }
    
    v = split->right;
    while (v) {
        if (high >= v->split_val) {
            if(inRangeQ(v->curr_point, Q, d, t)){
                out.push_back(v->curr_point);
                size++;
                color_count[v->curr_point->color_id]++;
            }
            if(level == total_levels-1){
                if(v->left) size+= v->left->subtree_size;
                if(v->left){
                    for(int i=0;i<numColors;i++) color_count[i]+= v->left->color_count[i];
                }
            }
            if(v->left && v->left->next_tree){
                dDRangeQuery(v->left->next_tree, Q, F, level + 1, total_levels, last_dim, d, t, out, size, color_count);
            }
            v = v->right;
        } else {
            v = v->left;
        }
    }
}


void side_expand_2(RangeTreeNode* root,  QueryBox& Q, FeatureBox& F, int level, int total_levels, int last_dim, int d, int t, vector<Point*>& out, int &size, vector<int> &color_count) {
    if (!root || level >= total_levels) return;

    int dim = (level < d) ? level : (level - d);
    if(level<d){
        dim = level >= last_dim ? level+1:level;
        if(level==total_levels-1)
            dim = last_dim;
    }
    double low = (level < d) ? Q.bounds[dim].first : F.bounds[dim].first;
    double high = (level < d) ? Q.bounds[dim].second : F.bounds[dim].second;

    RangeTreeNode* split = findSplitNode(root, low, high);
    if (!split) return;

    if(inRangeQExpand(split->curr_point, Q, d, t, last_dim)){
        out.push_back(split->curr_point);
        size++;
        color_count[split->curr_point->color_id]++;
    }
    // Traverse left path;
    RangeTreeNode* v = split->left;
    while (v) {
        if (low <= v->split_val) {
            if(inRangeQExpand(v->curr_point, Q, d, t, last_dim)){
                out.push_back(v->curr_point);
                size++;
                color_count[v->curr_point->color_id]++;
            }
            if(level == total_levels-1){
                if(v->right) size+= v->right->subtree_size;
                if(v->right){
                    for(int i=0;i<numColors;i++) color_count[i]+= v->right->color_count[i];
                }
            }
            if(v->right && v->right->next_tree){
                side_expand_2(v->right->next_tree, Q, F, level + 1, total_levels, last_dim, d, t, out, size, color_count);
            }
            v = v->left;
        } else {
            if(level == total_levels-1){
                out.push_back(v->curr_point);
                size++;
                color_count[v->curr_point->color_id]++;
            }
            v = v->right;
        }
    }
    v = split->right;
    while (v) {
        if (high >= v->split_val) {
            if(inRangeQExpand(v->curr_point, Q, d, t, last_dim)){
                out.push_back(v->curr_point);
                size++;
                color_count[v->curr_point->color_id]++;
            }
            if(level == total_levels-1){
                if(v->left) size+= v->left->subtree_size;
                if(v->left){
                    for(int i=0;i<numColors;i++) color_count[i]+= v->left->color_count[i];
                }
            }
            if(v->left && v->left->next_tree){
                side_expand_2(v->left->next_tree, Q, F, level + 1, total_levels, last_dim, d, t, out, size, color_count);
            }
            v = v->right;
        } else {
            if(level == total_levels-1){
                out.push_back(v->curr_point);
                size++;
                color_count[v->curr_point->color_id]++;
            }
            v = v->left;
        }
    }
}



vector<FeatureBox> splitFeatureBoxMasked(const FeatureBox &fbox, const Point &p, int mask) {
    vector<FeatureBox> subBoxes;
    
    for(int i = 0; i <d; i++) { // Generate t-1 sub-boxes for next skyline points
        FeatureBox box = fbox; 
        for(int j = 0; j < i; j++) {
            if (((mask >> j) & 1) == 0) {
                box.bounds[j].first = max(p.coords[j], box.bounds[j].first);
            } else {
                box.bounds[j].second = min(p.coords[j], box.bounds[j].second);
            }
        }
        
        if (((mask >> i) & 1) == 0) {
            box.bounds[i].second = min(p.coords[i] - 1e-9, box.bounds[i].second);
        } else {
            box.bounds[i].first = max(p.coords[i] + 1e-9, box.bounds[i].first);
        }
        
        bool valid = true;
        for (int j = 0; j < d; j++) {
            if (box.bounds[j].first > box.bounds[j].second) {
                valid = false;
                break;
            }
        }
        for(int j=0;j<d;j++){
            if(box.bounds[j].first == fbox.bounds[j].first && box.bounds[j].second == fbox.bounds[j].second){
                valid = false;
                break;
            }
        }
        if (valid) {
            subBoxes.push_back(box);
        }
    }
    
    return subBoxes;
}
vector<FeatureBox> splitFeatureBox(const FeatureBox &fbox, const Point &p) {
    vector<FeatureBox> subBoxes;
    for(int i=0; i<t-1; i++){ 
        FeatureBox box=fbox;    
        // box.bounds.resize(t);
        // for (int i = 0; i < t; i++) {
        //     box.bounds[i] = { -numeric_limits<double>::infinity(), numeric_limits<double>::infinity() };
        // }
        for(int j=0; j<i; j++){    
            box.bounds[j].first = max(p.coords[d + j],box.bounds[j].first);
        }
        box.bounds[i].second = min(p.coords[d + i] - 1e-9,box.bounds[i].second);
        box.bounds[t-1].first=max(p.coords[d + t - 1], box.bounds[t-1].first); 
        bool valid = true;
        for (int j = 0; j < t; j++) {
            if (box.bounds[j].first > box.bounds[j].second) {
                valid = false;
                break;
            }
        }
        
        if (valid) {
            subBoxes.push_back(box);
        }
    }
    return subBoxes;
}

vector<Point*> findSkyline(RangeTreeNode *root, const QueryBox &qbox, int mask) {
    FeatureBox fullBox;
    fullBox.bounds.resize(d);
    for (int i = 0; i < d; i++) {
        fullBox.bounds[i] = { qbox.bounds[i].first, qbox.bounds[i].second };
    }

    stack<FeatureBox> Z;
    Z.push(fullBox);

    unordered_set<int>reported;
    vector<Point*> skyline;
    // cout << "\nSkyline points (Range Tree Based):\n";
    // for(int i=0;i<d;i++){
    //     cout<<"["<<qbox.bounds[i].first<<","<<qbox.bounds[i].second<<"], ";
    // }
    // cout<<"\n";
    while (!Z.empty()) {
        FeatureBox R = Z.top(); Z.pop();
        QueryBox Rq;
        for(int i=0;i<d;i++){
            Rq.bounds.push_back({R.bounds[i].first,R.bounds[i].second});
        }
        vector<Point *> canonicals;
        // cout<<"Processing FeatureBox: [ ";
        // for (int i = 0; i < d; i++) {
        //     cout << R.bounds[i].first << ", " << R.bounds[i].second << " ";
        // }
        int size = 0;
        vector<int> temp(numColors,0);
        dDRangeQuery(root, const_cast<QueryBox &>(Rq), R, 0, d, d-1, d, t, canonicals,size, temp);
        Point *best = nullptr;
        for (auto pt : canonicals) {
            if (!inRangeBox(*pt, qbox)) continue;
            if (!best || min_all_masked(pt, best, mask)) {
                best = pt;
            }
        }

        if (best) {
            
            // cout << "Point " << best->index << ": Range [ ";
            // for (int i = 0; i < d; i++) cout << best->coords[i] << " ";
            // cout << "] Features [ ";
            // for (int i = 0; i < t; i++) cout << best->coords[d + i] << " ";
            // cout << "]\n";
            auto subBoxes = splitFeatureBoxMasked(R, *best,mask);
    
            for (auto it = subBoxes.rbegin(); it != subBoxes.rend(); ++it) {
                Z.push(*it);
            }
            if(reported.count(best->index)==0){
                skyline.push_back(best);
                reported.insert(best->index);
            }
            
        }
    }
    return skyline;
}

bool dominates(const Point& p, const Point& q) {
    bool strictlyBetter = false;
    for (int i = t-1; i >= 0; i--) {
        if (p.coords[d + i] > q.coords[d + i]) return false;
        if (p.coords[d + i] < q.coords[d + i]) strictlyBetter = true;
    }
    return strictlyBetter;
}

void bruteForceSkyline(const QueryBox& qbox) {
    vector<Point> candidates;
    for (auto& p : points) {
        if (inRangeBox(p, qbox)) {candidates.push_back(p);}
    }

    vector<Point> skyline;
    for (const auto& p : candidates) {
        bool dominated = false; // cout << "\nSkyline points (Brute-force method):\n";
    // for (const auto& p : skyline) {
    //     cout << "Point " << p.index << ": Range [ ";
    //     for (int i = 0; i < d; i++) cout << p.coords[i] << " ";
    //     cout << "] Features [ ";
    //     for (int i = 0; i < t; i++) cout << p.coords[d + i] << " ";
    //     cout << "]\n";
    // }
        for (const auto& q : candidates) {
            if (p.index != q.index && dominates(q, p)) {
                dominated = true;
                break;
            }
        }
        if (!dominated) skyline.push_back(p);
    }

    // cout << "\nSkyline points (Brute-force method):\n";
    // for (const auto& p : skyline) {
    //     cout << "Point " << p.index << ": Range [ ";
    //     for (int i = 0; i < d; i++) cout << p.coords[i] << " ";
    //     cout << "] Features [ ";
    //     for (int i = 0; i < t; i++) cout << p.coords[d + i] << " ";
    //     cout << "]\n";
    // }
}

bool isSamebound(QueryBox &Q1, QueryBox& Q2, int dim){
    for(int i=0;i<dim;i++){
        if(Q1.bounds[i].first!=Q2.bounds[i].first)return 0;
        if(Q1.bounds[i].second!=Q2.bounds[i].second)return 0;
    }
    return 1;
}

vector<QueryBox> generateCornerSkylineRanges(
    const QueryBox& qbox,
    const vector<double>& best_lhs,
    const vector<double>& best_rhs) {

    int d = qbox.bounds.size();
    vector<QueryBox> corner_ranges;
    QueryBox copybox = qbox;
    int total_masks = 1 << d;

    for (int mask = 0; mask < total_masks; ++mask) {
        QueryBox newbox = qbox;
        for (int i = 0; i < d; ++i) {
            if ((mask >> i) & 1) {
                // if (best_rhs[i] < 1e17){
                    newbox.bounds[i].first = newbox.bounds[i].second+1e-9;
                    newbox.bounds[i].second = best_rhs[i];
                // }
            } else {
                // if (best_lhs[i] > -1e17){
                    newbox.bounds[i].second = newbox.bounds[i].first-1e-9;
                    newbox.bounds[i].first = best_lhs[i];
                // }
            }
        }
        bool valid = true;
        for (int i = 0; i < d; ++i) {
            if (newbox.bounds[i].first > newbox.bounds[i].second) {
                valid = false;
                break;
            }
        }   
        if(isSamebound(newbox, copybox, d))valid=false;
        if (!valid) continue;
        corner_ranges.push_back(newbox);
    }

    return corner_ranges;
}



QueryBox intersectBoxes(const QueryBox& Q, const QueryBox& neighbor, int d, bool& valid) {
    QueryBox intersectBox;
    intersectBox.bounds.resize(d);
    valid = true;

    for (int i = 0; i < d; i++) {
        double low = std::max(Q.bounds[i].first, neighbor.bounds[i].first);
        double high = std::min(Q.bounds[i].second, neighbor.bounds[i].second);
        if (low > high) {
            valid = false; // no overlap in this dimension
            break;
        }
        intersectBox.bounds[i] = {low, high};
    }

    return intersectBox;
}

QueryBox unionBoxes(const QueryBox& Q, const QueryBox& neighbor, int d) {
    QueryBox unionBox;
    unionBox.bounds.resize(d);

    for (int i = 0; i < d; i++) {
        double low = std::min(Q.bounds[i].first, neighbor.bounds[i].first);
        double high = std::max(Q.bounds[i].second, neighbor.bounds[i].second);
        unionBox.bounds[i] = {low, high};
    }

    return unionBox;
}


vector<pair<double,double>> bfs(vector<RangeTreeNode *> &trees, QueryBox &qbox, RangeTreeNode* root){
    FeatureBox fullBox;
    unordered_set<std::vector<std::pair<double, double>>, HashVectorOfPairs> boxSet;
    fullBox.bounds.resize(t);
    for (int i = 0; i < t; i++) {
        fullBox.bounds[i] = { -numeric_limits<double>::infinity(), numeric_limits<double>::infinity() };
    }

    priority_queue<vector<double>>pq;
    vector<QueryBox>boxes;
    boxes.push_back(qbox);
    boxSet.insert(qbox.bounds);

    int base_size=0;
    vector<Point *> base_canonicals;
    vector<int> base_color_count(numColors,0);
    dDRangeQuery(root, const_cast<QueryBox &>(qbox), fullBox, 0, d, d-1, d, t, base_canonicals,base_size, base_color_count);
    
    vector<double> base_state = {1.0, 0.0};
    for (int i = 1; i < numColors; ++i) {
        base_state.push_back((double)base_color_count[i]);
    }
    pq.push(base_state);

    #if DEBUG
    cout<<"Base Color count: \n";
    for(int i=0;i<d;i++){
        cout<<"["<<qbox.bounds[i].first<<","<<qbox.bounds[i].second<<"], ";
    }
    cout<<"\n";
    for(int i=1;i<numColors;i++){
        cout<<base_color_count[i]<<" ";
    }
    cout<<"\n";
    #endif

    QueryBox intersect_box, union_box;
    while(!pq.empty()){
        auto priority = pq.top();
        double sim = priority[0];
        int pos = (int)priority[1];
        QueryBox box = boxes[pos];
        
        vector<int> current_priority_colors(numColors, 0);
        for (int i = 1; i < numColors; ++i) {
            current_priority_colors[i] = (int)priority[1 + i];
        }
        
        // if(box.bounds[0].first > 1000.0) { cout << "EXPLORING SHRUNK BOX! LHS: " << box.bounds[0].first << " Sim: " << sim << "\n"; }
        
        pq.pop();
        vector<Point*>cur_canonicals;
        int subtree_size=0;
        vector<int> color_count(numColors,0);
        dDRangeQuery(root, const_cast<QueryBox &>(box), fullBox, 0, d, d-1, d, t, cur_canonicals,subtree_size,color_count);
        #if DEBUG
        cout<<"Sim: "<<sim<<" subtree_size: "<<subtree_size<<"\n";
        cout<<"Current Color count: \n";
        for(int i=1;i<numColors;i++){
            cout<<color_count[i]<<" ";
        }
        cout<<"\n";
        #endif
        if(isFeasible(current_priority_colors)){
            vector<pair<double,double>>res;
            for(int i=0;i<d;i++){
                res.push_back({box.bounds.at(i).first,box.bounds.at(i).second}); 
            }
            res.push_back({sim,sim});
            return res;
        }
        
        vector<double> lhs_expands, rhs_expands;
        #if DEBUG
        cout<<"Current Box: \n";
        for(int i=0;i<d;i++){
            cout<<"["<<box.bounds[i].first<<" "<<box.bounds[i].second<<"], ";
        }
        cout<<"\n";
        #endif
        for (int dim = 0; dim < d; ++dim) {
            vector<Point*> out;
            int size=0;
            vector<int> temp(numColors,0);
            side_expand_2(trees[dim], box, fullBox, 0, d, dim, d, t,  out, size,temp);

            Point* lhs_shrink_pt_ptr = nullptr;
            Point* rhs_shrink_pt_ptr = nullptr;
            Point* lhs_expand_pt_ptr = nullptr;
            Point* rhs_expand_pt_ptr = nullptr;

            double lhs_bound=-1e16,rhs_bound = 1e16;
            double lhs_shrink_pt=1e16, rhs_shrink_pt=-1e16;
            double second_lhs_shrink_pt=1e16, second_rhs_shrink_pt=-1e16;
            for(auto pt : out) {
                double val = pt->coords[dim];
                if (val < box.bounds[dim].first) {
                    if (val > lhs_bound) {
                        lhs_bound = val;
                        lhs_expand_pt_ptr = pt;
                    }
                } else if (val > box.bounds[dim].second) {
                    if (val < rhs_bound) {
                        rhs_bound = val;
                        rhs_expand_pt_ptr = pt;
                    }
                } else{
                    if (val < lhs_shrink_pt) {
                        second_lhs_shrink_pt = lhs_shrink_pt;
                        lhs_shrink_pt = val;
                        lhs_shrink_pt_ptr = pt;
                    } else if (val < second_lhs_shrink_pt && val > lhs_shrink_pt) {
                        second_lhs_shrink_pt = val;
                    }
                    if (val > rhs_shrink_pt) {
                        second_rhs_shrink_pt = rhs_shrink_pt;
                        rhs_shrink_pt = val;
                        rhs_shrink_pt_ptr = pt;
                    } else if (val > second_rhs_shrink_pt && val < rhs_shrink_pt) {
                        second_rhs_shrink_pt = val;
                    }
                }
            }
                
            QueryBox lhs_expanded_box = box, rhs_expanded_box = box, lhs_shrink_box = box, rhs_shrink_box = box;
            vector<Point*> canonicals;
            int intersection_size, union_size;
            lhs_expands.push_back(lhs_bound);
            rhs_expands.push_back(rhs_bound);

            lhs_expanded_box.bounds[dim].first = lhs_bound;
            bool valid1=0;
            intersect_box = intersectBoxes(qbox,lhs_expanded_box,d,valid1);
            if(!isSamebound(lhs_expanded_box,box,d) && valid1){
                if(boxSet.count(lhs_expanded_box.bounds)==0){
                    int intersection_size = 0;
                    vector<Point*> temp;
                    vector<int> intersection_color_count(numColors,0);
                    dDRangeQuery(root, const_cast<QueryBox &>(intersect_box), fullBox, 0, d, d-1, d, t, temp,intersection_size, intersection_color_count);
                    
                    int newSize=0;
                    vector<int> canonicals_color_count(numColors,0);
                    vector<Point*> canonicals;
                    dDRangeQuery(root, const_cast<QueryBox &>(lhs_expanded_box), fullBox, 0, d, d-1, d, t, canonicals,newSize,canonicals_color_count);
                    
                    int true_union_size = newSize + base_size - intersection_size;
                    double cor_sim = true_union_size > 0 ? (double)intersection_size / (double)true_union_size : 0.0;
                    
                    boxes.push_back(lhs_expanded_box);
                    boxSet.insert(lhs_expanded_box.bounds);
                    vector<double> next_state = {cor_sim, (double)((int)boxes.size()-1)};
                    for (int i = 1; i < numColors; ++i) {
                        next_state.push_back((double)canonicals_color_count[i]);
                    }
                    pq.push(next_state);
                }
            }
            rhs_expanded_box.bounds[dim].second = rhs_bound;
            bool valid2=0;
            intersect_box = intersectBoxes(qbox,rhs_expanded_box,d,valid2);
            if(!isSamebound(rhs_expanded_box,box,d) && valid2){
                if(boxSet.count(rhs_expanded_box.bounds)==0){
                    int intersection_size = 0;
                    vector<Point*> temp;
                    vector<int> intersection_color_count(numColors,0);
                    dDRangeQuery(root, const_cast<QueryBox &>(intersect_box), fullBox, 0, d, d-1, d, t, temp,intersection_size, intersection_color_count);
                    
                    int newSize=0;
                    vector<int> canonicals_color_count(numColors,0);
                    vector<Point*> canonicals;
                    dDRangeQuery(root, const_cast<QueryBox &>(rhs_expanded_box), fullBox, 0, d, d-1, d, t, canonicals,newSize,canonicals_color_count);
                    
                    int true_union_size = newSize + base_size - intersection_size;
                    double cor_sim = true_union_size > 0 ? (double)intersection_size / (double)true_union_size : 0.0;
                    
                    boxes.push_back(rhs_expanded_box);
                    boxSet.insert(rhs_expanded_box.bounds);
                    vector<double> next_state = {cor_sim, (double)((int)boxes.size()-1)};
                    for (int i = 1; i < numColors; ++i) {
                        next_state.push_back((double)canonicals_color_count[i]);
                    }
                    pq.push(next_state);
                }
            }

            if (second_lhs_shrink_pt < 1e15) {
                lhs_shrink_box.bounds[dim].first = second_lhs_shrink_pt;
                bool valid3=0;
                intersect_box = intersectBoxes(qbox,lhs_shrink_box,d,valid3);
                if(!isSamebound(lhs_shrink_box,box,d) && valid3){
                    if(boxSet.count(lhs_shrink_box.bounds)==0){
                        int intersection_size = 0;
                        vector<Point*> temp;
                        vector<int> intersection_color_count(numColors,0);
                        dDRangeQuery(root, const_cast<QueryBox &>(intersect_box), fullBox, 0, d, d-1, d, t, temp,intersection_size, intersection_color_count);
                        
                        int newSize=0;
                        vector<int> canonicals_color_count(numColors,0);
                        vector<Point*> canonicals;
                        dDRangeQuery(root, const_cast<QueryBox &>(lhs_shrink_box), fullBox, 0, d, d-1, d, t, canonicals,newSize,canonicals_color_count);
                        
                        int true_union_size = newSize + base_size - intersection_size;
                        double cor_sim = true_union_size > 0 ? (double)intersection_size / (double)true_union_size : 0.0;
                        
                        boxes.push_back(lhs_shrink_box);
                        boxSet.insert(lhs_shrink_box.bounds);
                        vector<double> next_state = {cor_sim, (double)((int)boxes.size()-1)};
                        for (int i = 1; i < numColors; ++i) {
                            next_state.push_back((double)canonicals_color_count[i]);
                        }
                        pq.push(next_state);
                    }
                }
            }

            if (second_rhs_shrink_pt > -1e15) {
                rhs_shrink_box.bounds[dim].second = second_rhs_shrink_pt;
                bool valid4=0;
                intersect_box = intersectBoxes(qbox,rhs_shrink_box,d,valid4);
                if(!isSamebound(rhs_shrink_box,box,d) && valid4){
                    if(boxSet.count(rhs_shrink_box.bounds)==0){
                        int intersection_size = 0;
                        vector<Point*> temp;
                        vector<int> intersection_color_count(numColors,0);
                        dDRangeQuery(root, const_cast<QueryBox &>(intersect_box), fullBox, 0, d, d-1, d, t, temp,intersection_size, intersection_color_count);
                        
                        int newSize=0;
                        vector<int> canonicals_color_count(numColors,0);
                        vector<Point*> canonicals;
                        dDRangeQuery(root, const_cast<QueryBox &>(rhs_shrink_box), fullBox, 0, d, d-1, d, t, canonicals,newSize,canonicals_color_count);
                        
                        int true_union_size = newSize + base_size - intersection_size;
                        double cor_sim = true_union_size > 0 ? (double)intersection_size / (double)true_union_size : 0.0;
                        
                        boxes.push_back(rhs_shrink_box);
                        boxSet.insert(rhs_shrink_box.bounds);
                        vector<double> next_state = {cor_sim, (double)((int)boxes.size()-1)};
                        for (int i = 1; i < numColors; ++i) {
                            next_state.push_back((double)canonicals_color_count[i]);
                        }
                        pq.push(next_state);
                    }
                }
            }
        }
    
        vector<QueryBox> corner_boxes = generateCornerSkylineRanges(box, lhs_expands, rhs_expands);
        for (int i=0;i<corner_boxes.size();i++) {
            const auto & corner_box = corner_boxes[i];
            vector<Point*> skyline = findSkyline(root, corner_box, i);
            for (Point* pt : skyline) {
                QueryBox neighbor = box;
                for (int i = 0; i < d; ++i) {
                    neighbor.bounds[i].first = min(neighbor.bounds[i].first, pt->coords[i]);
                    neighbor.bounds[i].second = max(neighbor.bounds[i].second, pt->coords[i]);
                }
                bool valid=0;
                intersect_box = intersectBoxes(qbox,neighbor,d,valid);
                if(!isSamebound(neighbor,box,d) && valid){
                    if(boxSet.count(neighbor.bounds)==0){
                        int intersection_size = 0;
                        vector<Point*> temp;
                        vector<int> intersection_color_count(numColors,0);
                        dDRangeQuery(root, const_cast<QueryBox &>(intersect_box), fullBox, 0, d, d-1, d, t, temp,intersection_size, intersection_color_count);
                        
                        int newSize=0;
                        vector<int> canonicals_color_count(numColors,0);
                        vector<Point*> canonicals;
                        dDRangeQuery(root, const_cast<QueryBox &>(neighbor), fullBox, 0, d, d-1, d, t, canonicals,newSize,canonicals_color_count);
                        
                        int true_union_size = newSize + base_size - intersection_size;
                        double cor_sim = true_union_size > 0 ? (double)intersection_size / (double)true_union_size : 0.0;
                        
                        boxes.push_back(neighbor);
                        boxSet.insert(neighbor.bounds);
                        vector<double> next_state = {cor_sim, (double)((int)boxes.size()-1)};
                        for (int i = 1; i < numColors; ++i) {
                            next_state.push_back((double)canonicals_color_count[i]);
                        }
                        pq.push(next_state);
                    }
                }
            }
        }
    }
}

void bruteAlgo(QueryBox &Q, vector<Point *>& points, int t, int d) {
    int n = points.size();
    double max_sim = 0.0;
    QueryBox ans;
    ans.bounds.resize(d);
    QueryBox box;
    box.bounds.resize(d);

    vector<vector<double>> ptvalues(d, vector<double>(n));
    for (int i = 0; i < d; i++) {
        for (int j = 0; j < n; j++) {
            ptvalues[i][j] = points[j]->coords[i];
        }
        sort(ptvalues[i].begin(), ptvalues[i].end());  // sort for meaningful boxes
    }

    // ptrs1: start corner of box
    vector<int> ptrs1(d, 0);
    while (true) {
        // Check bounds
        bool done1 = false;
        for (int i = d - 1; i >= 0; i--) {
            if (ptrs1[i] >= n) {
                if (i == 0) {
                    done1 = true;
                    break;
                }
                ptrs1[i] = 0;
                ptrs1[i - 1]++;
            }
        }
        if (done1) break;

        // ptrs2: end corner of box
        vector<int> ptrs2 = ptrs1;
        while (true) {
            bool done2 = false;
            for (int i = d - 1; i >= 0; i--) {
                if (ptrs2[i] >= n) {
                    if (i == 0) {
                        done2 = true;
                        break;
                    }
                    ptrs2[i] = 0;
                    ptrs2[i - 1]++;
                }
            }
            if (done2) break;

            bool validBox = true;
            for (int k = 0; k < d; k++) {
                double lo = ptvalues[k][ptrs1[k]];
                double hi = ptvalues[k][ptrs2[k]];
                if (lo > hi) {
                    validBox = false;
                    break;
                }
                box.bounds[k] = {lo, hi};
            }

            if (validBox) {
                double union_size = 0.0;
                double intersection_size = 0.0;
                double curr_size = 0.0;

                for (int k = 0; k < n; k++) {
                    bool inQ = inRangeQ(points[k], Q, d, t);
                    bool inB = inRangeQ(points[k], box, d, t);
                    if (inQ && inB) intersection_size += 1.0;
                    if (inQ || inB) union_size += 1.0;
                    if (inB) curr_size += 1.0;
                }

                // cout << intersection_size << " " << union_size << " " << curr_size << "\n";

                if (curr_size >= 7 && union_size > 0.0) {
                    double sim = intersection_size / union_size;
                    if (sim > max_sim) {
                        max_sim = sim;
                        ans = box;
                    }
                }
            }

            ptrs2[d - 1]++;
        }

        ptrs1[d - 1]++;
    }

    cout << "Brute Ans Range: \n";
    for (int i = 0; i < d; i++) {
        cout << "[" << ans.bounds[i].first << " " << ans.bounds[i].second << "], ";
    }
    cout << "\nMax sim: " << max_sim << "\n";
}


#include <fstream>
// ...existing code...

int main(int argc, char* argv[]) {
    std::string dataset_file = "small_acs_3d_points_5000.txt";
    if (argc > 1) {
        dataset_file = argv[1];
    }
    std::ifstream fin(dataset_file);
    int n;
    fin >> n;

    fin >> d >> t;

    points.resize(n);
    for (int i = 0; i < n; i++) {
        points[i].coords.resize(d + t);
        for (int j = 0; j < d + t; j++) fin >> points[i].coords[j];
        points[i].index = i;
    }

    for (int i = 0; i < n; i++) {
        fin >> points[i].color_id;
    }

    int max_color_id = 0;
    for (int i = 0; i < n; i++) {
        if (points[i].color_id > max_color_id) {
            max_color_id = points[i].color_id;
        }
    }
    numColors = max_color_id + 1;

    constraints_matrix.assign(numColors, std::vector<PairwiseConstraint>(numColors));

    std::string constraints_file = "constraints.txt";
    if (argc > 2) {
        constraints_file = argv[2];
    }
    
    parse_constraints_file(constraints_file, numColors, color_weights, use_coverage, min_coverage, constraints_matrix);
    
    // Print parsed constraints to verify
    for (int i = 1; i < numColors; ++i) {
        if (use_coverage) cout << "Constraint loaded: Color " << i << " | Type: Coverage | Threshold: " << min_coverage[i] << "\n";
        for (int j = 1; j < numColors; ++j) {
            if (i == j) continue;
            if (constraints_matrix[i][j].type != 0) {
                cout << "Constraint loaded: Color " << i << " - Color " << j 
                     << " | Type: " << (constraints_matrix[i][j].type == 1 ? "Ratio" : "Difference")
                     << " | Threshold: " << constraints_matrix[i][j].threshold << "\n";
            }
        }
    }

    vector<Point *> point_ptrs(n);
    for (int i = 0; i < n; ++i) point_ptrs[i] = &points[i];

    auto tree_start = std::chrono::high_resolution_clock::now();
    RangeTreeNode *root = build_tree(point_ptrs, 0, d+t-1, d + t);
    vector<RangeTreeNode *> trees(d);
    for(int i=0;i<d;i++){
        trees[i]=build_tree(point_ptrs, 0, i, d);
    }
    auto tree_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tree_duration = tree_end - tree_start;
    QueryBox qbox;
    qbox.bounds.resize(d);
    for (int i = 0; i < d; i++) {
        fin >> qbox.bounds[i].first >> qbox.bounds[i].second;
    }
    
    std::vector<int> initial_color_count(numColors, 0);
    for(int i=0;i<n;i++){
        if(inRangeBox(points[i], qbox)){
            initial_color_count[points[i].color_id]++;
            if(points[i].color_id==1) orgCr++;
            if(points[i].color_id==2) orgCb++;
        }
    }
    cout << "Initial Color count in QBox: \n";
    for(int i=1;i<numColors;i++){
        cout << "Color " << i << ": " << initial_color_count[i] << " | ";
    }
    cout << "\n";
    // bruteAlgo(qbox,point_ptrs,t,d);
    vector<pair<double,double>> ans; 
    auto start = std::chrono::high_resolution_clock::now();
    ans = bfs(trees, qbox, root);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start; 
    std::cout << "Execution time: " << duration.count() << " seconds" << std::endl;
    std::cout << "Tree build time: " << tree_duration.count() << " seconds" << std::endl;
    cout << std::fixed << std::setprecision(6);
    cout << "Ans size "<<ans.size()<<"\n";
    for(int i=0;i<d;i++){
        cout<<"["<<ans[i].first<<","<<ans[i].second<<"], ";
    }
    cout<<"\nMax sim: "<<ans[d].first<<"\n";
    QueryBox final_box;
    final_box.bounds.resize(d);
    for(int i=0;i<d;i++){
        final_box.bounds[i].first=ans[i].first;
        final_box.bounds[i].second=ans[i].second;
    }
    cout<<"Points in final box:\n";
    vector<int> check_color_count(numColors, 0);
    for(int i=0;i<n;i++){
        if(inRangeQ(&points[i], final_box, d, t)){
            check_color_count[points[i].color_id]++;
        }   
    }
    for(int i=1;i<numColors;i++){
        cout<<"Color "<<i<<": "<<check_color_count[i]<<" ";
    }
    cout<<"\n";
    return 0;
}


