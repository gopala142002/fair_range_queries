# Compiler and Flags
CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# ==========================================
# Cross-Platform OS Detection
# ==========================================
ifeq ($(OS),Windows_NT)
    # Windows Settings
    EXT = .exe
    MKDIR = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
    RMDIR = if exist $(subst /,\,$(1)) rmdir /s /q $(subst /,\,$(1))
    EXEC_CMD = .\$(subst /,\,$(1))$(EXT)
else
    # Linux / macOS Settings
    EXT =
    MKDIR = mkdir -p $(1)
    RMDIR = rm -rf $(1)
    EXEC_CMD = ./$(1)$(EXT)
endif
# ==========================================

# 1. Find all shared modules (everything EXCEPT the main folder)
MODULE_SRCS = $(wildcard $(SRC_DIR)/common/*.cpp) \
              $(wildcard $(SRC_DIR)/bfs/*.cpp) \
              $(wildcard $(SRC_DIR)/brute/*.cpp) \
              $(filter-out $(SRC_DIR)/query/main_%.cpp, $(wildcard $(SRC_DIR)/query/*.cpp)) \
              $(wildcard $(SRC_DIR)/trees/*.cpp)

# 2. Object files for the shared modules
MODULE_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(MODULE_SRCS))

# 3. Find all entry points (files with a main() function)
MAIN_SRCS = $(wildcard $(SRC_DIR)/main/*.cpp)

# 4. Define the output executables
TARGETS = $(patsubst $(SRC_DIR)/main/%.cpp, $(BIN_DIR)/%$(EXT), $(MAIN_SRCS)) $(BIN_DIR)/main_range_cov$(EXT)

# Default target
all: $(TARGETS)

# Create directories dynamically based on OS
dirs:
	@$(call MKDIR,$(BIN_DIR))
	@$(call MKDIR,$(OBJ_DIR)/bfs)
	@$(call MKDIR,$(OBJ_DIR)/brute)
	@$(call MKDIR,$(OBJ_DIR)/common)
	@$(call MKDIR,$(OBJ_DIR)/main)
	@$(call MKDIR,$(OBJ_DIR)/query)
	@$(call MKDIR,$(OBJ_DIR)/trees)

# Rule to link EACH final executable (uses order-only prerequisite '| dirs')
$(BIN_DIR)/%$(EXT): $(OBJ_DIR)/main/%.o $(MODULE_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@
	@echo ========================================
	@echo Built executable: $@
	@echo ========================================

$(BIN_DIR)/main_range_cov$(EXT): $(OBJ_DIR)/query/main_range_cov.o $(MODULE_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@
	@echo ========================================
	@echo Built executable: $@
	@echo ========================================

# Compile each .cpp file into a .o object file (uses order-only prerequisite '| dirs')
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Convenience rule to build and run the main_bfs program immediately
run-bfs: all
	@echo Running main_bfs...
	@$(call EXEC_CMD,$(BIN_DIR)/main_bfs)

# Convenience rule to build and run the main_trees program immediately
run-trees: all
	@echo Running main_trees...
	@$(call EXEC_CMD,$(BIN_DIR)/main_trees)

# Convenience rule to build and run the main_opt program immediately
run-opt: all
	@echo Running main_opt...
	@$(call EXEC_CMD,$(BIN_DIR)/main_opt)

# Convenience rule to build and run the main_cov program immediately
run-cov: all
	@echo Running main_cov...
	@$(call EXEC_CMD,$(BIN_DIR)/main_cov)

# Clean up build artifacts safely based on OS
clean:
	@echo Cleaning up object files and executables...
	@$(call RMDIR,$(OBJ_DIR))
	@$(call RMDIR,$(BIN_DIR))

.PHONY: all dirs run-bfs clean

#How to run?
#make run-bfs - Runs the BFS algorithm
#make run-trees - Runs the Tree structures algorithm
#make run-opt - Runs the Optimization algorithm
#make run-cov - Runs the Coverage algorithm
