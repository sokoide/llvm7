#include <assert.h>
#include <limits.h>
#include <llvm-c/ExecutionEngine.h>
#include <stdio.h>
#include <string.h>

#include "generate.h"
#include "test_common.h"

// Structure to manage LLVM JIT execution environment initialization/cleanup
typedef struct {
    LLVMExecutionEngineRef engine;
    LLVMModuleRef module;
} LLVMTestContext;

// Initialize LLVM JIT execution environment
static int init_llvm_context(LLVMTestContext* ctx, LLVMModuleRef module) {
    char* error = NULL;

    // Initialize LLVM JIT (once per program)
    static int llvm_initialized = 0;
    if (!llvm_initialized) {
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();
        LLVMInitializeNativeAsmParser();
        llvm_initialized = 1;
    }

    // Create execution engine
    if (LLVMCreateExecutionEngineForModule(&ctx->engine, module, &error) != 0) {
        fprintf(stderr, "Failed to create execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        return -1;
    }

    ctx->module = module;
    return 0;
}

// Cleanup LLVM JIT execution environment
static void cleanup_llvm_context(LLVMTestContext* ctx) {
    if (ctx->engine) {
        LLVMDisposeExecutionEngine(ctx->engine);
    }
}

// Execute module generated from AST and return value
static int execute_module(LLVMTestContext* ctx, const char* func_name) {
    LLVMValueRef func = LLVMGetNamedFunction(ctx->module, func_name);
    if (!func) {
        fprintf(stderr, "Function '%s' not found\n", func_name);
        return -1;
    }

    // Execute function and get result
    LLVMGenericValueRef result = LLVMRunFunction(ctx->engine, func, 0, NULL);
    int ret_value = LLVMGenericValueToInt(result, 1);
    LLVMDisposeGenericValue(result);

    return ret_value;
}

// return 0
char* test_generate_return_zero() {
    printf("--- Test: return 0 ---\n");

    ReturnExpr ast = {.value = 0};
    LLVMModuleRef module = generate_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&ctx, "main");
    cleanup_llvm_context(&ctx);

    mu_assert("Expected 0", result == 0);
    return NULL;
}

// return 1
char* test_generate_return_one() {
    printf("--- Test: return 1 ---\n");

    ReturnExpr ast = {.value = 1};
    LLVMModuleRef module = generate_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&ctx, "main");
    cleanup_llvm_context(&ctx);

    mu_assert("Expected 1", result == 1);
    return NULL;
}

// return 42
char* test_generate_return_42() {
    printf("--- Test: return 42 ---\n");

    ReturnExpr ast = {.value = 42};
    LLVMModuleRef module = generate_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&ctx, "main");
    cleanup_llvm_context(&ctx);

    mu_assert("Expected 42", result == 42);
    return NULL;
}

// return -1 (negative value)
char* test_generate_return_negative() {
    printf("--- Test: return -1 ---\n");

    ReturnExpr ast = {.value = -1};
    LLVMModuleRef module = generate_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&ctx, "main");
    cleanup_llvm_context(&ctx);

    mu_assert("Expected -1", result == -1);
    return NULL;
}

// return INT_MAX (maximum value)
char* test_generate_return_max_int() {
    printf("--- Test: return INT_MAX ---\n");

    ReturnExpr ast = {.value = INT_MAX};
    LLVMModuleRef module = generate_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&ctx, "main");
    cleanup_llvm_context(&ctx);

    mu_assert("Expected INT_MAX", result == INT_MAX);
    return NULL;
}

// return 12345 (medium positive value)
char* test_generate_return_medium_positive() {
    printf("--- Test: return 12345 ---\n");

    ReturnExpr ast = {.value = 12345};
    LLVMModuleRef module = generate_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&ctx, "main");
    cleanup_llvm_context(&ctx);

    mu_assert("Expected 12345", result == 12345);
    return NULL;
}

// return INT_MIN (minimum value)
char* test_generate_return_min_int() {
    printf("--- Test: return INT_MIN ---\n");

    ReturnExpr ast = {.value = INT_MIN};
    LLVMModuleRef module = generate_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&ctx, "main");
    cleanup_llvm_context(&ctx);

    mu_assert("Expected INT_MIN", result == INT_MIN);
    return NULL;
}
