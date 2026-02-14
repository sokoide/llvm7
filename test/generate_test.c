#include <assert.h>
#include <limits.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <stdio.h>

typedef struct {
    int value;
} ReturnExpr;

extern LLVMModuleRef generate_code_to_module(ReturnExpr* ast);

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

// Run single return value test
static void run_return_test(const char* test_name, int expected_value, int actual_value) {
    printf("[%s] ", test_name);

    if (actual_value == expected_value) {
        printf("PASS - Expected: %d, Got: %d\n", expected_value, actual_value);
    } else {
        printf("FAIL - Expected: %d, Got: %d\n", expected_value, actual_value);
        assert(actual_value == expected_value && "Test failed");
    }
}

// Test 1: return 0
void test_return_zero() {
    printf("\n--- Test: return 0 ---\n");

    ReturnExpr ast = {.value = 0};
    LLVMModuleRef module = generate_code_to_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        return;
    }

    int result = execute_module(&ctx, "main");
    run_return_test("return_zero", 0, result);

    cleanup_llvm_context(&ctx);
}

// Test 2: return 1
void test_return_one() {
    printf("\n--- Test: return 1 ---\n");

    ReturnExpr ast = {.value = 1};
    LLVMModuleRef module = generate_code_to_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        return;
    }

    int result = execute_module(&ctx, "main");
    run_return_test("return_one", 1, result);

    cleanup_llvm_context(&ctx);
}

// Test 3: return 42
void test_return_42() {
    printf("\n--- Test: return 42 ---\n");

    ReturnExpr ast = {.value = 42};
    LLVMModuleRef module = generate_code_to_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        return;
    }

    int result = execute_module(&ctx, "main");
    run_return_test("return_42", 42, result);

    cleanup_llvm_context(&ctx);
}

// Test 4: return -1 (negative value)
void test_return_negative() {
    printf("\n--- Test: return -1 ---\n");

    ReturnExpr ast = {.value = -1};
    LLVMModuleRef module = generate_code_to_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        return;
    }

    int result = execute_module(&ctx, "main");
    run_return_test("return_negative", -1, result);

    cleanup_llvm_context(&ctx);
}

// Test 5: return INT_MAX (maximum value)
void test_return_max_int() {
    printf("\n--- Test: return INT_MAX ---\n");

    ReturnExpr ast = {.value = INT_MAX};
    LLVMModuleRef module = generate_code_to_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        return;
    }

    int result = execute_module(&ctx, "main");
    run_return_test("return_max_int", INT_MAX, result);

    cleanup_llvm_context(&ctx);
}

// Test 6: return 12345 (medium positive value)
void test_return_medium_positive() {
    printf("\n--- Test: return 12345 ---\n");

    ReturnExpr ast = {.value = 12345};
    LLVMModuleRef module = generate_code_to_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        return;
    }

    int result = execute_module(&ctx, "main");
    run_return_test("return_medium_positive", 12345, result);

    cleanup_llvm_context(&ctx);
}

// Test 7: return INT_MIN (minimum value)
void test_return_min_int() {
    printf("\n--- Test: return INT_MIN ---\n");

    ReturnExpr ast = {.value = INT_MIN};
    LLVMModuleRef module = generate_code_to_module(&ast);

    LLVMTestContext ctx = {0};
    if (init_llvm_context(&ctx, module) != 0) {
        return;
    }

    int result = execute_module(&ctx, "main");
    run_return_test("return_min_int", INT_MIN, result);

    cleanup_llvm_context(&ctx);
}

int main() {
    printf("========================================\n");
    printf("  LLVM JIT Compiler Test Suite\n");
    printf("  Purpose: Test that code generated from AST executes correctly\n");
    printf("========================================\n");

    // Run all tests
    test_return_zero();
    test_return_one();
    test_return_42();
    test_return_negative();
    test_return_medium_positive();
    test_return_max_int();
    test_return_min_int();

    printf("\n========================================\n");
    printf("  All tests completed successfully\n");
    printf("========================================\n");

    return 0;
}