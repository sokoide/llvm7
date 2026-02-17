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

- **src/main.c**: Entry point, parses source and generates LLVM IR dump
- **src/lex.c**: Lexical analysis (tokenizer)
- **src/parse.c**: AST parser (recursive descent)
- **src/codegen.c**: LLVM IR code generation from AST
- **src/variable.c**: Local variable management (LVar)
- **test/*.c**: JIT execution tests using LLVM MCJIT and a simple unit test framework

### Types

```c
struct Type {
    enum { INT, PTR } ty;
    Type* ptr_to;  // For PTR, points to the type being pointed to
};
```

Currently supports `int` and pointers (including nested pointers).

### LLVM C API Usage

- Module name: "sokoide_module"
- Function: Standard C function definitions returning `i32` or pointers
- Local Variables: Uses `LLVMBuildAlloca` and `LLVMBuildLoad2`/`LLVMBuildStore` for each local variable.
- Uses Opaque Pointers (default in newer LLVM versions).

### Testing Pattern

Tests follow this pattern:

1. Construct AST manually or using `parse_program()`/`parse_stmt()`
2. Call `generate_module()` to get `LLVMModuleRef`
3. Initialize LLVM JIT context (`init_llvm_context()`)
4. Execute with `execute_module()` (uses `LLVMRunFunction`)
5. Verify result matches expected value
6. Cleanup with `cleanup_llvm_context()` (disposes of execution engine and module)

### Build System Notes

- Compiler: clang with C99 standard
- Uses `llvm-config` for cflags and ldflags
- Object files are built in `build/` directory
- When adding new source files, update `C_SRCS` in Makefile and use `addprefix` pattern for `C_OBJS`
