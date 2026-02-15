# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Tiny C compiler experiment using LLVM C API. Generates LLVM IR from simple AST and supports JIT execution for testing.

## Development Commands

```bash
# Build and run main program (dumps LLVM IR to stdout)
make run

# Run all tests
make test

# Clean build artifacts
make clean

# Build only (without running)
make
```

## Architecture

### Source Structure

- **src/main.c**: Entry point, manually constructs AST and generates LLVM IR dump
- **src/generate.c**: LLVM IR code generation from AST
  - `generate_code()`: Dumps LLVM IR to stdout
  - `generate_code_to_module()`: Returns LLVMModuleRef for testing
- **test/generate_test.c**: JIT execution tests using LLVM MCJIT

### AST Definition

```c
typedef struct {
    int value;
} ReturnExpr;
```

Currently only supports return expressions with integer values. lex and parser are not yet implemented.

### LLVM C API Usage

- Module name: "sokoide_module"
- Function: `main()` returning `i32`
- Single basic block named "entry"
- Uses `LLVMConstInt()` for integer constants and `LLVMBuildRet()` for return

### Testing Pattern

Tests follow this pattern:

1. Construct AST manually
2. Call `generate_code_to_module()` to get LLVMModuleRef
3. Initialize LLVM JIT context (`init_llvm_context()`)
4. Execute with `execute_module()`
5. Verify result matches expected value
6. Cleanup with `cleanup_llvm_context()`

### Build System Notes

- Compiler: clang with C99 standard
- Uses `llvm-config` for cflags and ldflags
- Object files are built in `build/` directory
- When adding new source files, update `C_SRCS` in Makefile and use `addprefix` pattern for `C_OBJS`
