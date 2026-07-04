CXX      = g++
CXXFLAGS = -O2 -std=c++17 -Wall

IFLAGS   = -Isrc/common \
           -Isrc/trees \
           -Isrc/query \
           -Isrc/bfs \
           -Isrc/brute \
           -Isrc/query_2d

BIN      = bin


# ============================================================
# OS specific configurations
# ============================================================

ifeq ($(OS),Windows_NT)

    EXE = .exe

    $(shell if not exist $(BIN) mkdir $(BIN))

    RM_OBJ = del /Q /F $(BIN)\*.o 2>nul || ver>nul
    RM_BIN = del /Q /F $(BIN)\main_* 2>nul || ver>nul

    RUN_PREFIX = $(BIN)/

else

    EXE =

    $(shell mkdir -p $(BIN))

    RM_OBJ = rm -f $(BIN)/*.o
    RM_BIN = rm -f $(BIN)/main_*

    RUN_PREFIX = ./$(BIN)/

endif



# ============================================================
# Object files
# ============================================================


# --------------------- Common -------------------------------

COMMON_OBJ = \
    $(BIN)/common.o \
    $(BIN)/coverage.o


# --------------------- Trees --------------------------------

TREE_OBJ = \
    $(BIN)/kd_tree.o \
    $(BIN)/range_tree.o \
    $(BIN)/rtree.o \
    $(BIN)/rstar_tree.o \
    $(BIN)/xtree.o


# --------------------- 1D Query variants --------------------

QUERY_OBJ = \
    $(BIN)/kd_tree_opt.o \
    $(BIN)/kd_tree_cov.o \
    $(BIN)/kd_tree_opt_cov.o \
    $(BIN)/range_tree_opt.o \
    $(BIN)/range_tree_cov.o \
    $(BIN)/range_tree_opt_cov.o \
    $(BIN)/rtree_opt.o \
    $(BIN)/rtree_cov.o \
    $(BIN)/rtree_opt_cov.o \
    $(BIN)/rstar_tree_opt.o \
    $(BIN)/rstar_tree_cov.o \
    $(BIN)/rstar_tree_opt_cov.o \
    $(BIN)/xtree_opt.o \
    $(BIN)/xtree_cov.o \
    $(BIN)/xtree_opt_cov.o


# --------------------- BFS ----------------------------------

BFS_OBJ = \
    $(BIN)/bfs_fair.o \
    $(BIN)/bfs_fair_cov.o


# --------------------- Brute Force --------------------------

BRUTE_OBJ = \
    $(BIN)/brute.o \
    $(BIN)/brute_fair.o \
    $(BIN)/brute_fair_cov.o


# --------------------- 2D Query -----------------------------

QUERY_2D_OBJ = \
    $(BIN)/data_2d.o \
    $(BIN)/brute_2d.o \
    $(BIN)/bfs_fair_2d.o \
    $(BIN)/kd_tree_opt_2d.o \
    $(BIN)/brute_fair_cov_2d.o \
    $(BIN)/bfs_fair_cov_2d.o \
    $(BIN)/kd_tree_opt_cov_2d.o


ALL_OBJ = \
    $(COMMON_OBJ) \
    $(TREE_OBJ) \
    $(QUERY_OBJ) \
    $(BFS_OBJ) \
    $(BRUTE_OBJ) \
    $(QUERY_2D_OBJ)



# ============================================================
# Default target
# ============================================================

all: combined



# ============================================================
# Common
# ============================================================

$(BIN)/common.o: src/common/common.cpp src/common/common.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/coverage.o: src/common/coverage.cpp \
                   src/common/coverage.h \
                   src/common/common.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@



# ============================================================
# Trees
# ============================================================

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



# ============================================================
# 1D Query variants
# ============================================================


# --------------------- KD Tree ------------------------------

$(BIN)/kd_tree_opt.o: src/query/kd_tree_opt.cpp src/query/kd_tree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/kd_tree_cov.o: src/query/kd_tree_cov.cpp src/query/kd_tree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/kd_tree_opt_cov.o: src/query/kd_tree_opt_cov.cpp \
                         src/query/kd_tree_opt_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@



# --------------------- Range Tree ---------------------------

$(BIN)/range_tree_opt.o: src/query/range_tree_opt.cpp \
                         src/query/range_tree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/range_tree_cov.o: src/query/range_tree_cov.cpp \
                         src/query/range_tree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/range_tree_opt_cov.o: src/query/range_tree_opt_cov.cpp \
                             src/query/range_tree_opt_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@



# --------------------- R Tree -------------------------------

$(BIN)/rtree_opt.o: src/query/rtree_opt.cpp src/query/rtree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/rtree_cov.o: src/query/rtree_cov.cpp src/query/rtree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/rtree_opt_cov.o: src/query/rtree_opt_cov.cpp \
                        src/query/rtree_opt_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@



# --------------------- R* Tree ------------------------------

$(BIN)/rstar_tree_opt.o: src/query/rstar_tree_opt.cpp \
                         src/query/rstar_tree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/rstar_tree_cov.o: src/query/rstar_tree_cov.cpp \
                         src/query/rstar_tree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/rstar_tree_opt_cov.o: src/query/rstar_tree_opt_cov.cpp \
                             src/query/rstar_tree_opt_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@



# --------------------- X Tree -------------------------------

$(BIN)/xtree_opt.o: src/query/xtree_opt.cpp src/query/xtree_opt.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/xtree_cov.o: src/query/xtree_cov.cpp src/query/xtree_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/xtree_opt_cov.o: src/query/xtree_opt_cov.cpp \
                        src/query/xtree_opt_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@



# ============================================================
# BFS
# ============================================================

$(BIN)/bfs_fair.o: src/bfs/bfs_fair.cpp src/bfs/bfs_fair.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/bfs_fair_cov.o: src/bfs/bfs_fair_cov.cpp \
                       src/bfs/bfs_fair_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@



# ============================================================
# Brute Force
# ============================================================

$(BIN)/brute.o: src/brute/brute.cpp src/brute/brute.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/brute_fair.o: src/brute/brute_fair.cpp \
                     src/brute/brute_fair.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/brute_fair_cov.o: src/brute/brute_fair_cov.cpp \
                         src/brute/brute_fair_cov.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@



# ============================================================
# 2D Query Objects
# ============================================================

$(BIN)/data_2d.o: src/query_2d/data_2d.cpp \
                  src/query_2d/data_2d.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/brute_2d.o: src/query_2d/brute_2d.cpp \
                   src/query_2d/brute_2d.h \
                   src/query_2d/data_2d.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/bfs_fair_2d.o: src/query_2d/bfs_fair_2d.cpp \
                 src/query_2d/bfs_fair_2d.h \
                 src/query_2d/data_2d.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/kd_tree_opt_2d.o: src/query_2d/kd_tree_opt_2d.cpp \
                         src/query_2d/kd_tree_opt_2d.h \
                         src/query_2d/data_2d.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


# ============================================================
# 2D Coverage Objects
# ============================================================

$(BIN)/brute_fair_cov_2d.o: src/query_2d/brute_fair_cov_2d.cpp \
                            src/query_2d/brute_fair_cov_2d.h \
                            src/query_2d/data_2d.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/bfs_fair_cov_2d.o: src/query_2d/bfs_fair_cov_2d.cpp \
                          src/query_2d/bfs_fair_cov_2d.h \
                          src/query_2d/data_2d.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@


$(BIN)/kd_tree_opt_cov_2d.o: src/query_2d/kd_tree_opt_cov_2d.cpp \
                             src/query_2d/kd_tree_opt_cov_2d.h \
                             src/query_2d/data_2d.h
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

    
# ============================================================
# Individual 1D Executables
# ============================================================

$(BIN)/main_brute$(EXE): src/main/main_brute.cpp \
                         $(BIN)/brute.o \
                         $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_brute_fair$(EXE): src/main/main_brute_fair.cpp \
                              $(BIN)/brute_fair.o \
                              $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_brute_fair_cov$(EXE): src/main/main_brute_fair_cov.cpp \
                                  $(BIN)/brute_fair_cov.o \
                                  $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@



$(BIN)/main_kd$(EXE): src/main/main_kd.cpp \
                      $(BIN)/kd_tree.o \
                      $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_kd_opt$(EXE): src/main/main_kd_opt.cpp \
                          $(BIN)/kd_tree_opt.o \
                          $(BIN)/kd_tree.o \
                          $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_kd_cov$(EXE): src/main/main_kd_cov.cpp \
                          $(BIN)/kd_tree_cov.o \
                          $(BIN)/kd_tree.o \
                          $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@



$(BIN)/main_range$(EXE): src/main/main_range.cpp \
                         $(BIN)/range_tree.o \
                         $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_range_opt$(EXE): src/main/main_range_opt.cpp \
                             $(BIN)/range_tree_opt.o \
                             $(BIN)/range_tree.o \
                             $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_range_cov$(EXE): src/main/main_range_cov.cpp \
                             $(BIN)/range_tree_cov.o \
                             $(BIN)/range_tree.o \
                             $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@



$(BIN)/main_rtree$(EXE): src/main/main_rtree.cpp \
                         $(BIN)/rtree.o \
                         $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_rtree_opt$(EXE): src/main/main_rtree_opt.cpp \
                             $(BIN)/rtree_opt.o \
                             $(BIN)/rtree.o \
                             $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_rtree_cov$(EXE): src/main/main_rtree_cov.cpp \
                             $(BIN)/rtree_cov.o \
                             $(BIN)/rtree.o \
                             $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@



$(BIN)/main_rstar$(EXE): src/main/main_rstar.cpp \
                         $(BIN)/rstar_tree.o \
                         $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_rstar_opt$(EXE): src/main/main_rstar_opt.cpp \
                             $(BIN)/rstar_tree_opt.o \
                             $(BIN)/rstar_tree.o \
                             $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_rstar_cov$(EXE): src/main/main_rstar_cov.cpp \
                             $(BIN)/rstar_tree_cov.o \
                             $(BIN)/rstar_tree.o \
                             $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@



$(BIN)/main_xtree$(EXE): src/main/main_xtree.cpp \
                         $(BIN)/xtree.o \
                         $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_xtree_opt$(EXE): src/main/main_xtree_opt.cpp \
                             $(BIN)/xtree_opt.o \
                             $(BIN)/xtree.o \
                             $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_xtree_cov$(EXE): src/main/main_xtree_cov.cpp \
                             $(BIN)/xtree_cov.o \
                             $(BIN)/xtree.o \
                             $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@



$(BIN)/main_bfs_fair$(EXE): src/main/main_bfs_fair.cpp \
                            $(BIN)/bfs_fair.o \
                            $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_bfs_fair_cov$(EXE): src/main/main_bfs_fair_cov.cpp \
                                $(BIN)/bfs_fair_cov.o \
                                $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@



# ============================================================
# Unified 1D Executables
# ============================================================

$(BIN)/main_bfs_unified$(EXE): src/main/main_bfs.cpp \
                               $(BFS_OBJ) \
                               $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_trees_unified$(EXE): src/main/main_trees.cpp \
                                 $(TREE_OBJ) \
                                 $(QUERY_OBJ) \
                                 $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_coverage_unified$(EXE): src/main/main_coverage.cpp \
                                    $(TREE_OBJ) \
                                    $(QUERY_OBJ) \
                                    $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@


$(BIN)/main_brute_unified$(EXE): src/main/main_brute_combined.cpp \
                                 $(BRUTE_OBJ) \
                                 $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@



# ============================================================
# Unified 2D Executable
# ============================================================

$(BIN)/main_2d$(EXE): src/query_2d/main_2d.cpp \
                      $(QUERY_2D_OBJ) \
                      $(BIN)/kd_tree.o \
                      $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) $^ -o $@



# ============================================================
# Combined build targets
# ============================================================

combined: \
    $(BIN)/main_bfs_unified$(EXE) \
    $(BIN)/main_trees_unified$(EXE) \
    $(BIN)/main_coverage_unified$(EXE) \
    $(BIN)/main_brute_unified$(EXE) \
    $(BIN)/main_2d$(EXE)


# Build only the 1D framework

one_d: \
    $(BIN)/main_bfs_unified$(EXE) \
    $(BIN)/main_trees_unified$(EXE) \
    $(BIN)/main_coverage_unified$(EXE) \
    $(BIN)/main_brute_unified$(EXE)


# Build only the 2D framework

two_d: $(BIN)/main_2d$(EXE)



# ============================================================
# 1D Run Targets
# ============================================================

DATASET ?= data/40000_5

COLORS ?= 5

TREE_MODE ?= kd_opt

BFS_MODE ?= fair

BRUTE_MODE ?= base

COV_MODE ?= kd



run_trees: $(BIN)/main_trees_unified$(EXE)
	$(RUN_PREFIX)main_trees_unified$(EXE) \
	$(DATASET) \
	$(COLORS) \
	$(TREE_MODE)


run_bfs: $(BIN)/main_bfs_unified$(EXE)
	$(RUN_PREFIX)main_bfs_unified$(EXE) \
	$(DATASET) \
	$(COLORS) \
	$(BFS_MODE)


run_brute: $(BIN)/main_brute_unified$(EXE)
	$(RUN_PREFIX)main_brute_unified$(EXE) \
	$(DATASET) \
	$(COLORS) \
	$(BRUTE_MODE)


run_coverage: $(BIN)/main_coverage_unified$(EXE)
	$(RUN_PREFIX)main_coverage_unified$(EXE) \
	$(DATASET) \
	$(COLORS) \
	$(COV_MODE)


run: run_trees



# ============================================================
# 2D Run Configuration
# ============================================================

DATASET_2D ?= data/live_5d.txt

COLORS_2D ?= 3

DIM_X ?= 0

DIM_Y ?= 1

MODE_2D ?= kd_opt



# ============================================================
# 2D Run Target
# ============================================================

run_2d: $(BIN)/main_2d$(EXE)
	$(RUN_PREFIX)main_2d$(EXE) \
	$(DATASET_2D) \
	$(COLORS_2D) \
	$(DIM_X) \
	$(DIM_Y) \
	$(MODE_2D)



# ============================================================
# Clean
# ============================================================

clean:
	$(RM_OBJ)
	$(RM_BIN)



# ============================================================
# Phony targets
# ============================================================

.PHONY: \
    all \
    combined \
    one_d \
    two_d \
    run \
    run_trees \
    run_bfs \
    run_brute \
    run_coverage \
    run_2d \
    clean