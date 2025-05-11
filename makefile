# Compiler settings
CC = gcc
CFLAGS = -std=c11 -g
INCLUDE_FLAGS = -Iinclude

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Create build directory
$(shell mkdir -p $(BUILD_DIR))

# Source files
MAIN_SRC = main.c
SRCS = $(MAIN_SRC) $(wildcard $(SRC_DIR)/*.c)

# Object files (moved to build directory)
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))

# Dependency files (moved to build directory)
DEPS = $(OBJS:.o=.d)

# Target executable
TARGET = app

# Default target
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -o $@ $^ -lncurses	

# Compilation rule for main
$(BUILD_DIR)/main.o: main.c $(wildcard $(INCLUDE_DIR)/*.h)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -MMD -c $< -o $@

# Compilation rule for source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(INCLUDE_DIR)/*.h)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -MMD -c $< -o $@

# Include dependency files
-include $(DEPS)

# Clean up
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Phony targets
.PHONY: all clean
