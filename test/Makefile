# Test Makefile
TEST_TARGET = ../build/test_runner
CC = clang
CFLAGS = -Wall -Wextra -O2 -std=c99 -I../src `llvm-config --cflags`
LDFLAGS = `llvm-config --ldflags --libs --system-libs`

# Source files
TEST_SRC = $(shell find . -name '*.c')
PRODUCT_SRC = $(filter-out ../src/main.c, $(shell find ../src -name '*.c'))

# Build directory
BUILD_DIR = ../build

# Default target
.PHONY: all
all: $(TEST_TARGET)

# Build test executable
$(TEST_TARGET): $(BUILD_DIR) $(TEST_SRC) $(PRODUCT_SRC)
	@echo "Building test runner..."
	$(CC) $(CFLAGS) $(PRODUCT_SRC) $(TEST_SRC) -o $(TEST_TARGET) $(LDFLAGS)
	@chmod +x $(TEST_TARGET)

# Run tests
.PHONY: test
test: $(TEST_TARGET)
	@echo "Running tests..."
	$(TEST_TARGET)

.PHONY: format
format:
	clang-format -i *.c *.h

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TEST_TARGET)
