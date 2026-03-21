#include <assert.h>
#include <limits.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <stdio.h>
#include <string.h>

#include "../src/codegen.h"
#include "../src/lex.h"
#include "../src/parse.h"
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

    // Verify module
    char* verify_error = NULL;
    if (LLVMVerifyModule(module, LLVMReturnStatusAction, &verify_error)) {
        fprintf(stderr, "Module verification failed: %s\n", verify_error);
        LLVMDisposeMessage(verify_error);
        return -1;
    }

    // Create execution engine
    if (LLVMCreateExecutionEngineForModule(&ctx->engine, module, &error) != 0) {
        fprintf(stderr, "Failed to create execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        return -1;
    }

    // Map the alloc4 function from test_common.c
    LLVMValueRef alloc4_func = LLVMGetNamedFunction(module, "alloc4");
    if (alloc4_func) {
        LLVMAddGlobalMapping(ctx->engine, alloc4_func, (void*)&alloc4);
    }

    ctx->module = module;
    return 0;
}

// Cleanup LLVM JIT execution environment
static void cleanup_llvm_context(LLVMTestContext* ctx) {
    if (ctx->engine) {
        // This will also dispose of the module owned by the engine
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
    Node* ret = new_node(ND_RETURN, ast, NULL);
    ret->type = new_type_int();

    // Create function node wrapping the statement
    Token main_tok = {0};
    main_tok.kind = TK_IDENT;
    main_tok.str = "main";
    main_tok.len = 4;

    Node* func = new_node(ND_FUNCTION, ret, NULL);
    func->tok = &main_tok;
    func->type = new_type_int();

    // Setup Context with single function
    Context parse_ctx = {0};
    parse_ctx.code[0] = func;
    parse_ctx.code[1] = NULL;
    parse_ctx.node_count = 1;

    LLVMModuleRef module = generate_module(&parse_ctx);

    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        // If engine creation fails, we must dispose the module ourselves
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected %d, got %d", expected, result);
    mu_assert(msg, result == expected);
    free_ast(func);
    return NULL;
}

// Helper to run generate test returning double
static char* run_generate_double_test(Node* ast, double expected) {
    Node* ret = new_node(ND_RETURN, ast, NULL);
    ret->type = new_type_double();

    // Create function node wrapping the statement
    Token main_tok = {0};
    main_tok.kind = TK_IDENT;
    main_tok.str = "main";
    main_tok.len = 4;

    Node* func = new_node(ND_FUNCTION, ret, NULL);
    func->tok = &main_tok;
    func->type = new_type_double();

    // Setup Context with single function
    Context parse_ctx = {0};
    parse_ctx.code[0] = func;
    parse_ctx.code[1] = NULL;
    parse_ctx.node_count = 1;

    LLVMModuleRef module = generate_module(&parse_ctx);
    if (!module) {
        return "Failed to generate module";
    }

    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    if (LLVMCreateExecutionEngineForModule(&engine, module, &error)) {
        fprintf(stderr, "Failed to create execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        return "Failed to create execution engine";
    }

    double (*main_func)() =
        (double (*)(void))LLVMGetFunctionAddress(engine, "main");
    if (!main_func) {
        LLVMDisposeExecutionEngine(engine);
        return "Failed to get function address";
    }

    double result = main_func();

    LLVMDisposeExecutionEngine(engine);
    // Note: module is disposed by execution engine

    char msg[128];
    if (result != expected) {
        sprintf(msg, "Expected %f, got %f", expected, result);
        char* fail_msg = strdup(msg);
        return fail_msg;
    }

    return NULL;
}

// Helper to run generate test returning float
static char* run_generate_float_test(Node* ast, float expected) {
    Node* ret = new_node(ND_RETURN, ast, NULL);
    ret->type = new_type_float();

    // Create function node wrapping the statement
    Token main_tok = {0};
    main_tok.kind = TK_IDENT;
    main_tok.str = "main";
    main_tok.len = 4;

    Node* func = new_node(ND_FUNCTION, ret, NULL);
    func->tok = &main_tok;
    func->type = new_type_float();

    // Setup Context with single function
    Context parse_ctx = {0};
    parse_ctx.code[0] = func;
    parse_ctx.code[1] = NULL;
    parse_ctx.node_count = 1;

    LLVMModuleRef module = generate_module(&parse_ctx);
    if (!module) {
        return "Failed to generate module";
    }

    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    if (LLVMCreateExecutionEngineForModule(&engine, module, &error)) {
        fprintf(stderr, "Failed to create execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        return "Failed to create execution engine";
    }

    float (*main_func)() =
        (float (*)(void))LLVMGetFunctionAddress(engine, "main");
    if (!main_func) {
        LLVMDisposeExecutionEngine(engine);
        return "Failed to get function address";
    }

    float result = main_func();

    LLVMDisposeExecutionEngine(engine);

    char msg[128];
    // Use a small epsilon for float comparison
    float diff = result - expected;
    if (diff < 0)
        diff = -diff;
    if (diff > 0.00001f) {
        sprintf(msg, "Expected %f, got %f", (double)expected, (double)result);
        char* fail_msg = strdup(msg);
        return fail_msg;
    }

    return NULL;
}

char* test_generate_float_add() {
    Node* lhs = new_node_fnum(1.5, new_type_float());
    Node* rhs = new_node_fnum(2.5, new_type_float());
    Node* add = new_node(ND_ADD, lhs, rhs);
    add->type = new_type_float();

    return run_generate_float_test(add, 4.0f);
}

char* test_generate_float_sub() {
    Node* lhs = new_node_fnum(5.5, new_type_float());
    Node* rhs = new_node_fnum(1.2, new_type_float());
    Node* sub = new_node(ND_SUB, lhs, rhs);
    sub->type = new_type_float();

    return run_generate_float_test(sub, 4.3f);
}

char* test_generate_float_to_double() {
    Context ctx = {0};
    Token* head = tokenize("double main() { float x = 1.5f; return x; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    double (*main_func)() =
        (double (*)(void))LLVMGetFunctionAddress(engine, "main");
    double result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 1.5)
        return "Expected 1.5";
    return NULL;
}

char* test_generate_double_to_float() {
    Context ctx = {0};
    Token* head = tokenize("float main() { double x = 1.5; return x; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    float (*main_func)() =
        (float (*)(void))LLVMGetFunctionAddress(engine, "main");
    float result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 1.5f)
        return "Expected 1.5f";
    return NULL;
}

// return 42
char* test_generate_return_42() {
    Node* ast = new_node_num(42);
    return run_generate_test(ast, 42);
}

// return -1 (negative value)
char* test_generate_return_negative() {
    Node* ast = new_node_num(-1);
    return run_generate_test(ast, -1);
}

// return INT_MAX (maximum value)
char* test_generate_return_max_int() {
    Node* ast = new_node_num(INT_MAX);
    return run_generate_test(ast, INT_MAX);
}

// return INT_MIN (minimum value)
char* test_generate_return_min_int() {
    Node* ast = new_node_num(INT_MIN);
    return run_generate_test(ast, INT_MIN);
}

// return 1 + 2
char* test_generate_add() {
    Node* ast = new_node(ND_ADD, new_node_num(1), new_node_num(2));
    ast->type = new_type_int();
    return run_generate_test(ast, 3);
}

// return 5 - 3
char* test_generate_sub() {
    Node* ast = new_node(ND_SUB, new_node_num(5), new_node_num(3));
    ast->type = new_type_int();
    return run_generate_test(ast, 2);
}

// return 4 * 6
char* test_generate_mul() {
    Node* ast = new_node(ND_MUL, new_node_num(4), new_node_num(6));
    ast->type = new_type_int();
    return run_generate_test(ast, 24);
}

// return 10 / 2
char* test_generate_div() {
    Node* ast = new_node(ND_DIV, new_node_num(10), new_node_num(2));
    ast->type = new_type_int();
    return run_generate_test(ast, 5);
}

// return 2 + 3 * 4 (precedence test)
char* test_generate_precedence() {
    Node* mul = new_node(ND_MUL, new_node_num(3), new_node_num(4));
    mul->type = new_type_int();
    Node* ast = new_node(ND_ADD, new_node_num(2), mul);
    ast->type = new_type_int();
    return run_generate_test(ast, 14);
}

// return (2 + 3) * 4 (parentheses test)
char* test_generate_parentheses() {
    Node* add = new_node(ND_ADD, new_node_num(2), new_node_num(3));
    add->type = new_type_int();
    Node* ast = new_node(ND_MUL, add, new_node_num(4));
    ast->type = new_type_int();
    return run_generate_test(ast, 20);
}

// return 10 + 4 * 8 - 2 / 2
char* test_generate_complex() {
    Node* mul = new_node(ND_MUL, new_node_num(4), new_node_num(8));
    mul->type = new_type_int();
    Node* div = new_node(ND_DIV, new_node_num(2), new_node_num(2));
    div->type = new_type_int();
    Node* add = new_node(ND_ADD, new_node_num(10), mul);
    add->type = new_type_int();
    Node* ast = new_node(ND_SUB, add, div);
    ast->type = new_type_int();
    return run_generate_test(ast, 41);
}

// return 1 < 2
char* test_generate_lt_true() {
    Node* ast = new_node(ND_LT, new_node_num(1), new_node_num(2));
    ast->type = new_type_int();
    return run_generate_test(ast, 1);
}

// return 2 < 1
char* test_generate_lt_false() {
    Node* ast = new_node(ND_LT, new_node_num(2), new_node_num(1));
    ast->type = new_type_int();
    return run_generate_test(ast, 0);
}

// return 1 == 1
char* test_generate_eq_true() {
    Node* ast = new_node(ND_EQ, new_node_num(1), new_node_num(1));
    ast->type = new_type_int();
    return run_generate_test(ast, 1);
}

// return 1 != 1
char* test_generate_ne_false() {
    Node* ast = new_node(ND_NE, new_node_num(1), new_node_num(1));
    ast->type = new_type_int();
    return run_generate_test(ast, 0);
}

// Test that main function is required
// Note: This test is skipped because generate_module calls exit(1) on missing
// main
char* test_generate_main_required() {
    // Skip this test since it would cause the program to exit
    return NULL;
}

// Test pointer example from user prompt:
// int x; int *y; y = &x; *y = 3; return x;
char* test_generate_pointer_example() {
    Context ctx = {0};
    // Tokenize full program with main
    Token* head =
        tokenize("int main() { int x; int *y; y = &x; *y = 3; return x; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    // Note: module is owned by engine, so it's disposed in cleanup_llvm_context
    free_tokens(head);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected 3, got %d", result);
    mu_assert(msg, result == 3);
    return NULL;
}

char* test_generate_pointer_arithmetic() {
    Context ctx = {0};
    // Tokenize full program with main
    // int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q;
    Token* head = tokenize("int main() { int *p; alloc4(&p, 1, 2, 4, 8); int "
                           "*q; q = p + 2; return *q; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected 4, got %d", result);
    mu_assert(msg, result == 4);
    return NULL;
}

char* test_generate_pointer_arithmetic_sub() {
    Context ctx = {0};
    // int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 3; q = q - 1; return *q;
    Token* head = tokenize("int main() { int *p; alloc4(&p, 1, 2, 4, 8); int "
                           "*q; q = p + 3; q = q - 1; return *q; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected 4, got %d", result);
    mu_assert(msg, result == 4);
    return NULL;
}

char* test_generate_sizeof() {
    Context ctx = {0};
    // int x; int *y; return sizeof(x) + sizeof(y); // 4 + 8 = 12
    Token* head =
        tokenize("int main() { int x; int *y; return sizeof(x) + sizeof(y); }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected 12, got %d", result);
    mu_assert(msg, result == 12);
    return NULL;
}

char* test_generate_sizeof_expr() {
    Context ctx = {0};
    // int x; int *y; return sizeof(x + 3) + sizeof(y + 3) + sizeof(*y) +
    // sizeof(1) + sizeof(sizeof(1)); 4 + 8 + 4 + 4 + 4 = 24
    Token* head =
        tokenize("int main() { int x; int *y; return sizeof(x + 3) + sizeof(y "
                 "+ 3) + sizeof(*y) + sizeof(1) + sizeof(sizeof(1)); }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected 24, got %d", result);
    mu_assert(msg, result == 24);
    return NULL;
}

char* test_generate_array() {
    Context ctx = {0};
    // int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); // 1
    // + 2 = 3
    Token* head = tokenize("int main() { int a[2]; *a = 1; *(a + 1) = 2; int "
                           "*p; p = a; return *p + *(p + 1); }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected 3, got %d", result);
    mu_assert(msg, result == 3);
    return NULL;
}

char* test_generate_array_subscript() {
    Context ctx = {0};
    // int a[3]; a[0] = 10; a[1] = 20; a[2] = 30; return a[0] + a[1] + a[2];
    // 10 + 20 + 30 = 60
    Token* head =
        tokenize("int main() { int a[3]; a[0] = 10; a[1] = 20; a[2] = 30; "
                 "return a[0] + a[1] + a[2]; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected 60, got %d", result);
    mu_assert(msg, result == 60);
    return NULL;
}

char* test_generate_array_subscript_reversed() {
    Context ctx = {0};
    // int a[2]; a[0] = 5; a[1] = 7; return 0[a] + 1[a]; // 5 + 7 = 12
    // Note: 0[a] is equivalent to a[0] in C
    Token* head = tokenize("int main() { int a[2]; a[0] = 5; a[1] = 7; return "
                           "0[a] + 1[a]; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }

    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);

    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected 12, got %d", result);
    mu_assert(msg, result == 12);

    return NULL;
}

char* test_generate_global_var() {
    Context ctx = {0};
    Token* head = tokenize("int g; int main() { g = 42; return g; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 42", result == 42);
    return NULL;
}

char* test_generate_global_ptr_var() {
    Context ctx = {0};
    Token* head =
        tokenize("int *g; int main() { int x; x = 7; g = &x; return *g; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 7", result == 7);
    return NULL;
}

char* test_generate_global_array() {
    Context ctx = {0};
    Token* head = tokenize(
        "int g[3]; int main() { *g = 10; *(g+1) = 20; g[2] = 30; return "
        "g[0] + g[1] + g[2]; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 60", result == 60);
    return NULL;
}

char* test_generate_global_array_subscript() {
    Context ctx = {0};
    Token* head = tokenize(
        "int g[2]; int main() { g[0] = 5; g[1] = 7; return 0[g] + 1[g]; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 12", result == 12);
    return NULL;
}

char* test_generate_global_ptr_return_func() {
    Context ctx = {0};
    Token* head = tokenize("int g; int *get_g() { return &g; } int main() { g "
                           "= 123; return *get_g(); }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 123", result == 123);
    return NULL;
}

char* test_generate_char_array() {
    Context ctx = {0};
    // char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y; → 3
    Token* head =
        tokenize("int main() { char x[3]; x[0] = -1; x[1] = 2; int y; "
                 "y = 4; return x[0] + y; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 3", result == 3);
    return NULL;
}

char* test_generate_string_literal() {
    Context ctx = {0};
    // char *s; s = "ABC"; return *s; → 65 (ASCII 'A')
    Token* head = tokenize("int main() { char *s; s = \"ABC\"; return *s; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 65 (ASCII A)", result == 65);
    return NULL;
}

char* test_generate_struct() {
    Context ctx = {0};
    // struct { int a; int b; int* c; } s; s.a = 1; s.b = 2; s.c = &s.a; return
    // s.a + s.b + *s.c; -> 4
    Token* head = tokenize("int main() { struct { int a; int b; int* c; } s; "
                           "s.a = 1; s.b = 2; s.c "
                           "= &s.a; return s.a + s.b + *s.c; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 4", result == 4);
    return NULL;
}

char* test_generate_struct_simple() {
    Context ctx = {0};
    // struct { int a; } s; s.a = 42; return s.a; -> 42
    Token* head =
        tokenize("int main() { struct { int a; } s; s.a = 42; return s.a; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 42", result == 42);
    return NULL;
}

char* test_generate_struct_assign() {
    Context ctx = {0};
    // struct { int a; int b; } s; s.a = 1; s.b = 2; return s.a + s.b; -> 3
    Token* head = tokenize("int main() { struct { int a; int b; } s; s.a = 1; "
                           "s.b = 2; return s.a + s.b; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 3", result == 3);
    return NULL;
}

char* test_generate_char_simple() {
    Context ctx = {0};
    // char x; x = 127; return x; -> 127
    Token* head = tokenize("int main() { char x; x = 127; return x; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 127", result == 127);
    return NULL;
}

char* test_generate_comments() {
    Context ctx = {0};
    // return 1; // comment\n /* comment */ return 2; -> 1 (if first return hit)
    // or 2 Actually testing that comments don't break parsing.
    Token* head =
        tokenize("int main() { // comment\n return /* block */ 42; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 42", result == 42);
    return NULL;
}

char* test_generate_sizeof_types() {
    Context ctx = {0};
    // sizeof(char c)=1 + sizeof(int)=4 + sizeof(int*)=8
    // + sizeof(int a[5])=20 + sizeof(struct{int,int})=8 = 41
    Token* head =
        tokenize("int main() { char c; int a[5]; struct { int a; int b; } s; "
                 "return sizeof(c) + sizeof(int) + sizeof(int*) + sizeof(a) + "
                 "sizeof(s); }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    printf("DEBUG: sizeof results = %d\n", result);
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    static char msg[64];
    snprintf(msg, sizeof(msg), "Expected 41, got %d", result);
    mu_assert(msg, result == 41);
    return NULL;
}

char* test_generate_logic() {
    Context ctx = {0};
    // (1 && 2) + (0 && 1) + (0 || 1) + (1 || 2) + (!0) + (!5)
    // 1 + 0 + 1 + 1 + 1 + 0 = 4
    Token* head = tokenize("int main() { return (1 && 2) + (0 && 1) + (0 || 1) "
                           "+ (1 || 2) + (!0) + (!5); }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 4", result == 4);
    return NULL;
}

char* test_generate_char_literal() {
    Context ctx = {0};
    // return 'a'; -> 97
    // return '\n'; -> 10
    // return 'z' - 'a'; -> 25
    // return '\0' == 0; -> 1
    // Total = 97 + 10 + 25 + 1 = 133
    Token* head = tokenize(
        "int main() { return 'a' + '\\n' + ('z' - 'a') + ('\\0' == 0); }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 133", result == 133);
    return NULL;
}

char* test_generate_arrow() {
    Context ctx = {0};
    // struct { int a; int b; } s; struct { int a; int b; } *p; p = &s; p->a =
    // 10; p->b = 20; return p->a + p->b; -> 30
    Token* head = tokenize(
        "int main() { struct { int a; int b; } s; struct { int a; int b; } *p; "
        "p = &s; p->a = 10; p->b = 20; return p->a + p->b; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 30", result == 30);
    return NULL;
}

char* test_generate_typedef() {
    Context ctx = {0};
    Token* head =
        tokenize("typedef int myint; typedef struct { int x; } S; int main() { "
                 "myint a; a = 42; S s; s.x = a; return s.x; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    init_llvm_context(&llvm_ctx, module);
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 42", result == 42);
    return NULL;
}

char* test_generate_enum() {
    Context ctx = {0};
    Token* head = tokenize("enum { A, B, C }; enum { X=10, Y, Z }; int main() "
                           "{ return B + Y; }"); // 1 + 11 = 12
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    init_llvm_context(&llvm_ctx, module);
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 12", result == 12);
    return NULL;
}

char* test_generate_builtin_const() {
    Context ctx = {0};
    Token* head = tokenize("int main() { int a; int b; int c; a = NULL; b = "
                           "false; c = true; return a + b + c; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    init_llvm_context(&llvm_ctx, module);
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 1", result == 1);
    return NULL;
}

char* test_generate_switch() {
    Context ctx = {0};
    Token* head = tokenize(
        "int main() { "
        "  int x; x = 2; int y; y = 0; "
        "  switch(x) { "
        "    case 1: y = 10; break; "
        "    case 2: y = 20; break; "
        "    default: y = 30; "
        "  } "
        "  int z; z = 0; "
        "  switch(2) { "
        "    case 1: "
        "    case 2: z = 100; " // fall-through intended? No, case 2 matches.
        "    case 3: z = z + 1; break; "
        "  } "
        "  return y + z; " // 20 + 101 = 121
        "}");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    init_llvm_context(&llvm_ctx, module);
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 121", result == 121);
    return NULL;
}

char* test_generate_switch_distinct_case_values() {
    Context ctx = {0};
    Token* head = tokenize("int main() { "
                           "  int x; x = 3; "
                           "  int y; y = 0; "
                           "  switch (x) { "
                           "    case 1: y = 10; break; "
                           "    case 2: y = 20; break; "
                           "    case 3: y = 30; break; "
                           "    default: y = 99; "
                           "  } "
                           "  return y; "
                           "}");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    init_llvm_context(&llvm_ctx, module);
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 30", result == 30);
    return NULL;
}

char* test_generate_switch_case_after_return_case() {
    Context ctx = {0};
    Token* head = tokenize("int main() { "
                           "  int x; x = 2; "
                           "  int y; y = 0; "
                           "  switch (x) { "
                           "    case 1: return 11; "
                           "    case 2: y = 22; break; "
                           "    default: y = 33; "
                           "  } "
                           "  return y; "
                           "}");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    init_llvm_context(&llvm_ctx, module);
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 22", result == 22);
    return NULL;
}

char* test_generate_inc_dec() {
    // test inc/dec operations
    Context ctx = {0};
    Token* head =
        tokenize("int main() { "
                 "  int i; i = 0; "
                 "  int a; a = ++i; " // a=1, i=1
                 "  int b; b = i++; " // b=1, i=2
                 "  int c; c = --i; " // c=1, i=1
                 "  int d; d = i--; " // d=1, i=0
                 "  int arr[2]; arr[0] = 10; arr[1] = 20; "
                 "  int *p; p = arr; "
                 "  int x; x = *p++; "              // x=10, p=arr+1
                 "  int y; y = *p; "                // y=20
                 "  return a + b + c + d + x + y; " // 1+1+1+1+10+20 = 34
                 "}");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    init_llvm_context(&llvm_ctx, module);
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 34", result == 34);
    return NULL;
}

char* test_generate_proto_and_init() {
    Context ctx = {0};
    Token* head = tokenize("int foo(int x); "
                           "int gx = 10; "
                           "int* gpx = &gx; "
                           "char* gs = \"abc\"; "
                           "int main() { "
                           "  if (gx != 10) return 1; "
                           "  if (*gpx != 10) return 2; "
                           "  if (gs[0] != 97) return 3; "
                           "  int x = (int)gx; "
                           "  char y = (char)x; "
                           "  return foo(y); "
                           "} "
                           "int foo(int x) { return x + 5; }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");

    for (int i = 0; i < ctx.node_count; i++) {
        free_ast(ctx.code[i]);
    }
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 15", result == 15);
    return NULL;
}

char* test_generate_double_add() {
    Node* lhs = new_node_fnum(1.5, new_type_double());
    Node* rhs = new_node_fnum(2.5, new_type_double());
    Node* add = new_node(ND_ADD, lhs, rhs);
    add->type = new_type_double();

    return run_generate_double_test(add, 4.0);
}

char* test_generate_double_sub() {
    Node* lhs = new_node_fnum(5.5, new_type_double());
    Node* rhs = new_node_fnum(1.2, new_type_double());
    Node* sub = new_node(ND_SUB, lhs, rhs);
    sub->type = new_type_double();

    return run_generate_double_test(sub, 4.3);
}

char* test_generate_double_mul() {
    Node* lhs = new_node_fnum(2.0, new_type_double());
    Node* rhs = new_node_fnum(3.5, new_type_double());
    Node* mul = new_node(ND_MUL, lhs, rhs);
    mul->type = new_type_double();

    return run_generate_double_test(mul, 7.0);
}

char* test_generate_double_div() {
    Node* lhs = new_node_fnum(10.0, new_type_double());
    Node* rhs = new_node_fnum(4.0, new_type_double());
    Node* div = new_node(ND_DIV, lhs, rhs);
    div->type = new_type_double();

    return run_generate_double_test(div, 2.5);
}

char* test_generate_double_compare() {
    Node* lhs = new_node_fnum(1.2, new_type_double());
    Node* rhs = new_node_fnum(3.4, new_type_double());
    Node* lt = new_node(ND_LT, lhs, rhs);
    lt->type = new_type_int();

    return run_generate_test(lt, 1);
}

char* test_generate_double_from_int() {
    Context ctx = {0};
    Token* head = tokenize("double main() { double x = 42; return x; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    double (*main_func)() =
        (double (*)(void))LLVMGetFunctionAddress(engine, "main");
    double result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 42.0)
        return "Expected 42.0";
    return NULL;
}

char* test_generate_int_from_double() {
    Context ctx = {0};
    Token* head = tokenize("int main() { int x = 42.5; return x; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 42)
        return "Expected 42";
    return NULL;
}

char* test_generate_do_while() {
    // Standard case
    {
        Context ctx = {0};
        Token* head = tokenize("int main() { int i = 0; do { i = i + 1; } "
                               "while (i < 10); return i; }");
        ctx.current_token = head;
        parse_program(&ctx);

        LLVMModuleRef module = generate_module(&ctx);
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        LLVMExecutionEngineRef engine;
        char* error = NULL;
        LLVMCreateExecutionEngineForModule(&engine, module, &error);
        int (*main_func)() =
            (int (*)(void))LLVMGetFunctionAddress(engine, "main");
        int result = main_func();
        LLVMDisposeExecutionEngine(engine);

        for (int i = 0; i < ctx.node_count; i++)
            free_ast(ctx.code[i]);
        free_tokens(head);

        if (result != 10)
            return "Expected 10";
    }

    // Condition false initially - should execute once
    {
        Context ctx = {0};
        Token* head = tokenize(
            "int main() { int i = 0; do { i = i + 1; } while (0); return i; }");
        ctx.current_token = head;
        parse_program(&ctx);

        LLVMModuleRef module = generate_module(&ctx);
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        LLVMExecutionEngineRef engine;
        char* error = NULL;
        LLVMCreateExecutionEngineForModule(&engine, module, &error);
        int (*main_func)() =
            (int (*)(void))LLVMGetFunctionAddress(engine, "main");
        int result = main_func();
        LLVMDisposeExecutionEngine(engine);

        for (int i = 0; i < ctx.node_count; i++)
            free_ast(ctx.code[i]);
        free_tokens(head);

        if (result != 1)
            return "Expected 1 (should execute once)";
    }

    return NULL;
}

char* test_generate_ternary_float() {
    Context ctx = {0};
    Token* head =
        tokenize("float main() { float x = 1.5f; return 1 ? x : 2.5f; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    float (*main_func)() =
        (float (*)(void))LLVMGetFunctionAddress(engine, "main");
    float result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 1.5f)
        return "Expected 1.5f";
    return NULL;
}

char* test_generate_ternary_mixed() {
    Context ctx = {0};
    Token* head = tokenize("double main() { return 0 ? 1.5f : 2.5; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    double (*main_func)() =
        (double (*)(void))LLVMGetFunctionAddress(engine, "main");
    double result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 2.5)
        return "Expected 2.5";
    return NULL;
}

char* test_generate_ternary_int_double_common_type() {
    Context ctx = {0};
    Token* head = tokenize("double main() { return 0 ? 1 : 2.5; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    double (*main_func)() =
        (double (*)(void))LLVMGetFunctionAddress(engine, "main");
    double result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 2.5)
        return "Expected 2.5 (common type should be double)";
    return NULL;
}

char* test_generate_unsigned() {
    // 4294967295u / 2u
    {
        Context ctx = {0};
        Token* head =
            tokenize("unsigned int main() { return 4294967295u / 2u; }");
        ctx.current_token = head;
        parse_program(&ctx);

        LLVMModuleRef module = generate_module(&ctx);
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        LLVMExecutionEngineRef engine;
        char* error = NULL;
        LLVMCreateExecutionEngineForModule(&engine, module, &error);
        unsigned int (*main_func)() =
            (unsigned int (*)(void))LLVMGetFunctionAddress(engine, "main");
        unsigned int result = main_func();
        LLVMDisposeExecutionEngine(engine);

        for (int i = 0; i < ctx.node_count; i++)
            free_ast(ctx.code[i]);
        free_tokens(head);

        if (result != 2147483647u)
            return "Expected 2147483647u";
    }

    // -1 < 1u -> false (0)
    {
        Context ctx = {0};
        Token* head = tokenize("int main() { return -1 < 1u; }");
        ctx.current_token = head;
        parse_program(&ctx);

        LLVMModuleRef module = generate_module(&ctx);
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        LLVMExecutionEngineRef engine;
        char* error = NULL;
        LLVMCreateExecutionEngineForModule(&engine, module, &error);
        int (*main_func)() =
            (int (*)(void))LLVMGetFunctionAddress(engine, "main");
        int result = main_func();
        LLVMDisposeExecutionEngine(engine);

        for (int i = 0; i < ctx.node_count; i++)
            free_ast(ctx.code[i]);
        free_tokens(head);

        if (result != 0)
            return "Expected -1 < 1u to be false (0)";
    }

    return NULL;
}

char* test_generate_bitwise() {
    // 1 | 2 ^ 3 & 4
    // 3 & 4 = 0
    // 2 ^ 0 = 2
    // 1 | 2 = 3
    {
        Context ctx = {0};
        Token* head = tokenize("int main() { return 1 | 2 ^ 3 & 4; }");
        ctx.current_token = head;
        parse_program(&ctx);

        LLVMModuleRef module = generate_module(&ctx);
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        LLVMExecutionEngineRef engine;
        char* error = NULL;
        LLVMCreateExecutionEngineForModule(&engine, module, &error);
        int (*main_func)() =
            (int (*)(void))LLVMGetFunctionAddress(engine, "main");
        int result = main_func();
        LLVMDisposeExecutionEngine(engine);

        for (int i = 0; i < ctx.node_count; i++)
            free_ast(ctx.code[i]);
        free_tokens(head);

        if (result != 3)
            return "Expected 3 for bitwise precedence";
    }

    // Unary ~: ~0
    {
        Context ctx = {0};
        Token* head = tokenize("int main() { return ~0; }");
        ctx.current_token = head;
        parse_program(&ctx);

        LLVMModuleRef module = generate_module(&ctx);
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        LLVMExecutionEngineRef engine;
        char* error = NULL;
        LLVMCreateExecutionEngineForModule(&engine, module, &error);
        int (*main_func)() =
            (int (*)(void))LLVMGetFunctionAddress(engine, "main");
        int result = main_func();
        LLVMDisposeExecutionEngine(engine);

        for (int i = 0; i < ctx.node_count; i++)
            free_ast(ctx.code[i]);
        free_tokens(head);

        if (result != -1)
            return "Expected -1 for ~0";
    }

    // Shift: 1 << 2 + 3 -> 1 << 5 = 32
    {
        Context ctx = {0};
        Token* head = tokenize("int main() { return 1 << 2 + 3; }");
        ctx.current_token = head;
        parse_program(&ctx);

        LLVMModuleRef module = generate_module(&ctx);
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        LLVMExecutionEngineRef engine;
        char* error = NULL;
        LLVMCreateExecutionEngineForModule(&engine, module, &error);
        int (*main_func)() =
            (int (*)(void))LLVMGetFunctionAddress(engine, "main");
        int result = main_func();
        LLVMDisposeExecutionEngine(engine);

        for (int i = 0; i < ctx.node_count; i++)
            free_ast(ctx.code[i]);
        free_tokens(head);

        if (result != 32)
            return "Expected 32 for 1 << 2 + 3";
    }

    return NULL;
}

char* test_generate_compound_bitwise() {
    // int x = 3; x &= 1; -> 1
    {
        Context ctx = {0};
        Token* head = tokenize("int main() { int x = 3; x &= 1; return x; }");
        ctx.current_token = head;
        parse_program(&ctx);

        LLVMModuleRef module = generate_module(&ctx);
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        LLVMExecutionEngineRef engine;
        char* error = NULL;
        LLVMCreateExecutionEngineForModule(&engine, module, &error);
        int (*main_func)() =
            (int (*)(void))LLVMGetFunctionAddress(engine, "main");
        int result = main_func();
        LLVMDisposeExecutionEngine(engine);

        for (int i = 0; i < ctx.node_count; i++)
            free_ast(ctx.code[i]);
        free_tokens(head);

        if (result != 1)
            return "Expected 1 for 3 &= 1";
    }

    // int x = 1; x <<= 2; -> 4
    {
        Context ctx = {0};
        Token* head = tokenize("int main() { int x = 1; x <<= 2; return x; }");
        ctx.current_token = head;
        parse_program(&ctx);

        LLVMModuleRef module = generate_module(&ctx);
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();

        LLVMExecutionEngineRef engine;
        char* error = NULL;
        LLVMCreateExecutionEngineForModule(&engine, module, &error);
        int (*main_func)() =
            (int (*)(void))LLVMGetFunctionAddress(engine, "main");
        int result = main_func();
        LLVMDisposeExecutionEngine(engine);

        for (int i = 0; i < ctx.node_count; i++)
            free_ast(ctx.code[i]);
        free_tokens(head);

        if (result != 4)
            return "Expected 4 for 1 <<= 2";
    }

    return NULL;
}

char* test_generate_union_overlap() {
    Context ctx = {0};
    Token* head = tokenize("int main() { union { int i; char c; } u; u.i = 0; "
                           "u.c = 1; return u.i; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 1)
        return "Expected 1 for union overlap";
    return NULL;
}

char* test_generate_bitfield_access() {
    Context ctx = {0};
    Token* head =
        tokenize("int main() { struct { unsigned int a:3; unsigned int "
                 "b:5; } s; s.a = 5; s.b = 17; return s.a + s.b; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 22)
        return "Expected 22 for bitfield access";
    return NULL;
}

char* test_generate_goto_label() {
    Context ctx = {0};
    Token* head =
        tokenize("int main() { int x = 0; goto L; x = 1; L: return x; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 0)
        return "Expected 0 for goto/label";
    return NULL;
}

char* test_generate_designated_initializer_array() {
    Context ctx = {0};
    Token* head = tokenize("int main() { int a[3] = { [2] = 3, [0] = 1 }; "
                           "return a[0] + a[1] + a[2]; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 4)
        return "Expected 4 for designated array initializer";
    return NULL;
}

char* test_generate_designated_initializer_struct() {
    Context ctx = {0};
    Token* head = tokenize("int main() { struct { int a; int b; } s = { .b = "
                           "5, .a = 2 }; return s.a + s.b; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 7)
        return "Expected 7 for designated struct initializer";
    return NULL;
}

char* test_generate_long_double() {
    Context ctx = {0};
    Token* head = tokenize("int main() { long double x = 1.5; long double y = "
                           "2.25; return (int)(x + y); }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 3)
        return "Expected 3 for long double arithmetic";
    return NULL;
}

char* test_generate_scalar_compound_literal() {
    Context ctx = {0};
    Token* head = tokenize("int main() { return (int){3} + (int){4}; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 7)
        return "Expected 7 for scalar compound literal";
    return NULL;
}

char* test_generate_complex_basic() {
    Context ctx = {0};
    Token* head =
        tokenize("int main() { double _Complex x = 3.5; return (int)x; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 3)
        return "Expected 3 for basic _Complex handling";
    return NULL;
}

char* test_generate_vla_basic() {
    Context ctx = {0};
    Token* head = tokenize("int main() { int n = 3; int a[n]; a[0] = 7; a[2] = "
                           "5; return a[0] + a[2]; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 12)
        return "Expected 12 for VLA basic usage";
    return NULL;
}

char* test_generate_adjacent_string_literals() {
    Context ctx = {0};
    Token* head = tokenize(
        "int main() { char* s = \"foo\" \"bar\"; return s[0] + s[5]; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != ('f' + 'r'))
        return "Expected 'f'+'r' for adjacent string literals";
    return NULL;
}

char* test_generate_struct_compound_literal_member() {
    Context ctx = {0};
    Token* head =
        tokenize("int main() { return ((struct { int a; int b; }){1, 2}).b; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 2)
        return "Expected 2 for struct compound literal member";
    return NULL;
}

char* test_generate_array_compound_literal_subscript() {
    Context ctx = {0};
    Token* head = tokenize("int main() { return ((int[3]){3, 4, 5})[1]; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 4)
        return "Expected 4 for array compound literal subscript";
    return NULL;
}

char* test_generate_union_compound_literal_designator() {
    Context ctx = {0};
    Token* head = tokenize(
        "int main() { return ((union { int a; char b; }){ .b = 5 }).b; }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 5)
        return "Expected 5 for union compound literal designator";
    return NULL;
}

char* test_generate_sizeof_vla_expr_runtime() {
    Context ctx = {0};
    Token* head =
        tokenize("int main() { int n = 3; int a[n]; return sizeof(a); }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 12)
        return "Expected 12 for sizeof(VLA) runtime behavior";
    return NULL;
}

char* test_generate_function_pointer_basic() {
    Context ctx = {0};
    Token* head = tokenize("int inc(int x) { return x + 1; } int main() { int "
                           "(*fp)(int); fp = inc; "
                           "return fp(5); }");
    ctx.current_token = head;
    parse_program(&ctx);

    LLVMModuleRef module = generate_module(&ctx);
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMExecutionEngineRef engine;
    char* error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);
    int (*main_func)() = (int (*)(void))LLVMGetFunctionAddress(engine, "main");
    int result = main_func();
    LLVMDisposeExecutionEngine(engine);

    for (int i = 0; i < ctx.node_count; i++)
        free_ast(ctx.code[i]);
    free_tokens(head);

    if (result != 6)
        return "Expected 6 for basic function pointer call";
    return NULL;
}

char* test_generate_bool_basic() {
    Context ctx = {0};
    Token* head = tokenize("int main() { _Bool b = 5; _Bool z = 0; return b + "
                           "z + sizeof(_Bool); }");
    ctx.current_token = head;
    parse_program(&ctx);
    LLVMModuleRef module = generate_module(&ctx);
    LLVMTestContext llvm_ctx = {0};
    if (init_llvm_context(&llvm_ctx, module) != 0) {
        LLVMDisposeModule(module);
        free_tokens(head);
        return "Failed to initialize LLVM context";
    }
    int result = execute_module(&llvm_ctx, "main");
    cleanup_llvm_context(&llvm_ctx);
    free_tokens(head);
    mu_assert("Expected 2 (1 + 0 + 1)", result == 2);
    return NULL;
}
