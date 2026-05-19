CXX      = g++
CXXFLAGS = -O2 -std=c++17 -Wall
IFLAGS   = -Isrc/common -Isrc/trees -Isrc/query -Isrc/bfs -Isrc/brute
BIN      = bin

# Create bin directory
$(shell mkdir -p $(BIN))

# ===================== Object files =====================

COMMON_OBJ = $(BIN)/common.o $(BIN)/coverage.o
TREE_OBJ   = $(BIN)/kd_tree.o $(BIN)/range_tree.o $(BIN)/rtree.o \
             $(BIN)/rstar_tree.o $(BIN)/xtree.o
QUERY_OBJ  = $(BIN)/kd_tree_opt.o $(BIN)/kd_tree_cov.o \
             $(BIN)/range_tree_opt.o $(BIN)/range_tree_cov.o \
             $(BIN)/rtree_opt.o $(BIN)/rtree_cov.o \
             $(BIN)/rstar_tree_opt.o $(BIN)/rstar_tree_cov.o \
             $(BIN)/xtree_opt.o $(BIN)/xtree_cov.o
BFS_OBJ    = $(BIN)/bfs_fair.o $(BIN)/bfs_fair_cov.o
BRUTE_OBJ  = $(BIN)/brute.o $(BIN)/brute_fair.o $(BIN)/brute_fair_cov.o

ALL_OBJ    = $(COMMON_OBJ) $(TREE_OBJ) $(QUERY_OBJ) $(BFS_OBJ) $(BRUTE_OBJ)

# ===================== All targets =====================

all: $(BIN)/main_brute $(BIN)/main_brute_fair $(BIN)/main_brute_fair_cov \
     $(BIN)/main_kd $(BIN)/main_kd_opt $(BIN)/main_kd_cov \
     $(BIN)/main_range $(BIN)/main_range_opt $(BIN)/main_range_cov \
     $(BIN)/main_rtree $(BIN)/main_rtree_opt $(BIN)/main_rtree_cov \
     $(BIN)/main_rstar $(BIN)/main_rstar_opt $(BIN)/main_rstar_cov \
     $(BIN)/main_xtree $(BIN)/main_xtree_opt $(BIN)/main_xtree_cov \
     $(BIN)/main_bfs_fair $(BIN)/main_bfs_fair_cov \
     $(BIN)/convert_dataset

# ===================== Common =====================

$(BIN)/common.o: src/common/common.cpp src/common/common.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/coverage.o: src/common/coverage.cpp src/common/coverage.h src/common/common.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

# ===================== Trees =====================

$(BIN)/kd_tree.o: src/trees/kd_tree.cpp src/trees/kd_tree.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/range_tree.o: src/trees/range_tree.cpp src/trees/range_tree.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/rtree.o: src/trees/rtree.cpp src/trees/rtree.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/rstar_tree.o: src/trees/rstar_tree.cpp src/trees/rstar_tree.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/xtree.o: src/trees/xtree.cpp src/trees/xtree.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

# ===================== Query variants =====================

$(BIN)/kd_tree_opt.o: src/query/kd_tree_opt.cpp src/query/kd_tree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/kd_tree_cov.o: src/query/kd_tree_cov.cpp src/query/kd_tree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/range_tree_opt.o: src/query/range_tree_opt.cpp src/query/range_tree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/range_tree_cov.o: src/query/range_tree_cov.cpp src/query/range_tree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/rtree_opt.o: src/query/rtree_opt.cpp src/query/rtree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/rtree_cov.o: src/query/rtree_cov.cpp src/query/rtree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/rstar_tree_opt.o: src/query/rstar_tree_opt.cpp src/query/rstar_tree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/rstar_tree_cov.o: src/query/rstar_tree_cov.cpp src/query/rstar_tree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/xtree_opt.o: src/query/xtree_opt.cpp src/query/xtree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/xtree_cov.o: src/query/xtree_cov.cpp src/query/xtree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

# ===================== BFS =====================

$(BIN)/bfs_fair.o: src/bfs/bfs_fair.cpp src/bfs/bfs_fair.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/bfs_fair_cov.o: src/bfs/bfs_fair_cov.cpp src/bfs/bfs_fair_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

# ===================== Brute force =====================

$(BIN)/brute.o: src/brute/brute.cpp src/brute/brute.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/brute_fair.o: src/brute/brute_fair.cpp src/brute/brute_fair.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

$(BIN)/brute_fair_cov.o: src/brute/brute_fair_cov.cpp src/brute/brute_fair_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

# ===================== Executables =====================

$(BIN)/main_brute: src/main/main_brute.cpp $(BIN)/brute.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_brute_fair: src/main/main_brute_fair.cpp $(BIN)/brute_fair.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_brute_fair_cov: src/main/main_brute_fair_cov.cpp $(BIN)/brute_fair_cov.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_kd: src/main/main_kd.cpp $(BIN)/kd_tree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_kd_opt: src/main/main_kd_opt.cpp $(BIN)/kd_tree_opt.o $(BIN)/kd_tree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_kd_cov: src/main/main_kd_cov.cpp $(BIN)/kd_tree_cov.o $(BIN)/kd_tree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_range: src/main/main_range.cpp $(BIN)/range_tree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_range_opt: src/main/main_range_opt.cpp $(BIN)/range_tree_opt.o $(BIN)/range_tree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_range_cov: src/main/main_range_cov.cpp $(BIN)/range_tree_cov.o $(BIN)/range_tree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_rtree: src/main/main_rtree.cpp $(BIN)/rtree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_rtree_opt: src/main/main_rtree_opt.cpp $(BIN)/rtree_opt.o $(BIN)/rtree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_rtree_cov: src/main/main_rtree_cov.cpp $(BIN)/rtree_cov.o $(BIN)/rtree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_rstar: src/main/main_rstar.cpp $(BIN)/rstar_tree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_rstar_opt: src/main/main_rstar_opt.cpp $(BIN)/rstar_tree_opt.o $(BIN)/rstar_tree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_rstar_cov: src/main/main_rstar_cov.cpp $(BIN)/rstar_tree_cov.o $(BIN)/rstar_tree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_xtree: src/main/main_xtree.cpp $(BIN)/xtree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_xtree_opt: src/main/main_xtree_opt.cpp $(BIN)/xtree_opt.o $(BIN)/xtree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_xtree_cov: src/main/main_xtree_cov.cpp $(BIN)/xtree_cov.o $(BIN)/xtree.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_bfs_fair: src/main/main_bfs_fair.cpp $(BIN)/bfs_fair.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/main_bfs_fair_cov: src/main/main_bfs_fair_cov.cpp $(BIN)/bfs_fair_cov.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@

$(BIN)/convert_dataset: tools/convert_dataset.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# ===================== Clean =====================

clean:
	rm -f $(BIN)/*.o $(BIN)/main_* $(BIN)/convert_dataset
