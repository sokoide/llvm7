#include <assert.h>
#include <limits.h>
#include <llvm-c/ExecutionEngine.h>
#include <stdio.h>
#include <string.h>

#include "../src/ast.h"
#include "../src/generate.h"
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

// Helper to run generate test
static char* run_generate_test(Node* ast, int expected) {
    LLVMModuleRef module = generate_module(ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&ctx, "main");
    cleanup_llvm_context(&ctx);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected %d, got %d", expected, result);
    mu_assert(msg, result == expected);
    return NULL;
}

// return 42
char* test_generate_return_42() {
    Node* ast = new_node_num(42);
    char* result = run_generate_test(ast, 42);
    free(ast);
    return result;
}

// return -1 (negative value)
char* test_generate_return_negative() {
    Node* ast = new_node_num(-1);
    char* result = run_generate_test(ast, -1);
    free(ast);
    return result;
}

// return INT_MAX (maximum value)
char* test_generate_return_max_int() {
    Node* ast = new_node_num(INT_MAX);
    char* result = run_generate_test(ast, INT_MAX);
    free(ast);
    return result;
}

// return INT_MIN (minimum value)
char* test_generate_return_min_int() {
    Node* ast = new_node_num(INT_MIN);
    char* result = run_generate_test(ast, INT_MIN);
    free(ast);
    return result;
}

// return 1 + 2
char* test_generate_add() {
    Node* ast = new_node(ND_ADD, new_node_num(1), new_node_num(2));
    char* result = run_generate_test(ast, 3);
    free(ast->rhs);
    free(ast->lhs);
    free(ast);
    return result;
}

// return 5 - 3
char* test_generate_sub() {
    Node* ast = new_node(ND_SUB, new_node_num(5), new_node_num(3));
    char* result = run_generate_test(ast, 2);
    free(ast->rhs);
    free(ast->lhs);
    free(ast);
    return result;
}

// return 4 * 6
char* test_generate_mul() {
    Node* ast = new_node(ND_MUL, new_node_num(4), new_node_num(6));
    char* result = run_generate_test(ast, 24);
    free(ast->rhs);
    free(ast->lhs);
    free(ast);
    return result;
}

// return 10 / 2
char* test_generate_div() {
    Node* ast = new_node(ND_DIV, new_node_num(10), new_node_num(2));
    char* result = run_generate_test(ast, 5);
    free(ast->rhs);
    free(ast->lhs);
    free(ast);
    return result;
}

// return 2 + 3 * 4 (precedence test)
char* test_generate_precedence() {
    // 3 * 4 = 12, then 2 + 12 = 14
    Node* mul = new_node(ND_MUL, new_node_num(3), new_node_num(4));
    Node* ast = new_node(ND_ADD, new_node_num(2), mul);
    char* result = run_generate_test(ast, 14);
    free(ast->rhs);
    free(ast->lhs);
    free(ast);
    return result;
}

// return (2 + 3) * 4 (parentheses test)
char* test_generate_parentheses() {
    // 2 + 3 = 5, then 5 * 4 = 20
    Node* add = new_node(ND_ADD, new_node_num(2), new_node_num(3));
    Node* ast = new_node(ND_MUL, add, new_node_num(4));
    char* result = run_generate_test(ast, 20);
    free(ast->rhs);
    free(ast->lhs->rhs);
    free(ast->lhs->lhs);
    free(ast->lhs);
    free(ast);
    return result;
}

// return 10 + 4 * 8 - 2 / 2
char* test_generate_complex() {
    // 4 * 8 = 32
    Node* mul = new_node(ND_MUL, new_node_num(4), new_node_num(8));
    // 2 / 2 = 1
    Node* div = new_node(ND_DIV, new_node_num(2), new_node_num(2));
    // 10 + 32 = 42
    Node* add = new_node(ND_ADD, new_node_num(10), mul);
    // 42 - 1 = 41
    Node* ast = new_node(ND_SUB, add, div);
    char* result = run_generate_test(ast, 41);
    free(ast->rhs->rhs);
    free(ast->rhs->lhs);
    free(ast->rhs);
    free(ast->lhs->rhs);
    free(ast->lhs->lhs);
    free(ast->lhs);
    free(ast);
    return result;
}
