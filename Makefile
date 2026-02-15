# Tiny-C Compiler Makefile
TARGET = ./build/llvm7
CC = clang
CFLAGS = -Wall -Wextra -O2 -std=c99 -Isrc `llvm-config --cflags`
LDFLAGS = `llvm-config --ldflags --libs --system-libs`

# Directory structure
BUILD_DIR = build
TMP_DIR = tmp
SRC_DIR = src

# Source files
C_SRCS = src/main.c src/generate.c src/file.c
C_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SRCS:.c=.o)))
# LL_FILES = $(TMP_DIR)/$(notdir $(C_SRCS:.c=.ll))
# S_FILES = $(TMP_DIR)/$(notdir $(LL_FILES:.ll=.s))

# Default target
.PHONY: all
all: $(TARGET)

# Generate LLVM IR target
# .PHONY: llvm
# llvm: $(LL_FILES)

# # Generate .s
# .PHONY: llc
# llc: $(S_FILES)

# .PHONY: lld
# lld: $(TARGET)

.PHONY: run
run: $(TARGET)
	$(TARGET)

.PHONY: test
test:
	$(MAKE) -C test test

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TMP_DIR) $(TARGET)
	$(MAKE) -C test clean

.PHONY: format
format:
	clang-format -i $(SRC_DIR)/*.c $(SRC_DIR)/*.h demo/*.c demo/*.h
	$(MAKE) -C test format

# TARGET
$(TARGET): $(BUILD_DIR) $(C_OBJS)
	@echo "Linking $(TARGET)..."
	@echo "C_OBJS: $(C_OBJS)"
	$(CC) $(CFLAGS) -o $(TARGET) $(C_OBJS) $(LIBS) $(LDFLAGS)
	@chmod +x $(TARGET)

# Object file dependencies
# $(BUILD_DIR)/main.o: $(SRC_DIR)/main.c |  $(BUILD_DIR)
# 	@echo $(CC) $(CFLAGS) -Isrc -c $< -o $@
# 	$(CC) $(CFLAGS) -Isrc -c $< -o $@

# $(BUILD_DIR)/generate.o: $(SRC_DIR)/generate.c |  $(BUILD_DIR)
# 	@echo $(CC) $(CFLAGS) -Isrc -c $< -o $@
# 	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo $(CC) $(CFLAGS) -Isrc -c $< -o $@
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

# Directories

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

# TARGET
# $(TARGET): $(S_FILES) | $(BUILD_DIR)
# 	@echo "Generating $(TARGET)"
# 	clang -fuse-ld=lld $(S_FILES) -o $(TARGET)

# .c -> .ll
# $(TMP_DIR)/%.ll: $(SRC_DIR)/%.c | $(TMP_DIR)
# 	@echo "Generating LLVM IR: $@"
# 	clang -S -emit-llvm $(CFLAGS) -Isrc $< -o $@

# .ll -> .s
# $(TMP_DIR)/%.s: $(TMP_DIR)/*.ll
# 	@echo "Generating .s: $@"
# 	llc $< -o $@
