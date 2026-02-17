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
    // Create function node wrapping the statement
    Token main_tok = {0};
    main_tok.kind = TK_IDENT;
    main_tok.str = "main";
    main_tok.len = 4;

    Node* func = new_node(ND_FUNCTION, ast, NULL);
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
