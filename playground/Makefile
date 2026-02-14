# Tiny-C Compiler Makefile
TARGET = ./playground_exe
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LIBS =

# Source files
C_SRCS = main.c
LL_FILES = $(C_SRCS:.c=.ll)
S_FILES = $(LL_FILES:.ll=.s)

# Default target
.PHONY: all
all: $(TARGET)

# Generate LLVM IR target
.PHONY: llvm
llvm: $(LL_FILES)

# Generate .s
.PHONY: llc
llc: $(S_FILES)

.PHONY: lld
lld: $(TARGET)

.PHONY: clean
clean:
	rm -rf $(LL_FILES) $(S_FILES) $(TARGET)

# .c -> .ll
%.ll: %.c
	@echo "Generating LLVM IR: $@"
	clang -S -emit-llvm $(CFLAGS) $< -o $@

# .ll -> .s
%.s: %.ll
	@echo "Generating .s: $@"
	llc $< -o $@

# TARGET
$(TARGET): $(S_FILES)
	@echo "Generating $(TARGET)"
	clang -fuse-ld=lld $(S_FILES) -o $(TARGET)