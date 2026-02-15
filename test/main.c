#include "generate_test.h"
#include "lexer_test.h"
#include "test_common.h"
#include <stdio.h>

static char* run_all_tests() {
    mu_run_test(test_generate_return_zero);
    mu_run_test(test_generate_return_one);
    mu_run_test(test_generate_return_42);
    mu_run_test(test_generate_return_negative);
    mu_run_test(test_generate_return_medium_positive);
    mu_run_test(test_generate_return_max_int);
    mu_run_test(test_generate_return_min_int);
    mu_run_test(test_lexer_tokenize);
    return NULL;
}

int main() {
    printf("========================================\n");
    printf("  LLVM JIT Compiler Test Suite\n");
    printf("  Purpose: Test that code generated from AST executes correctly\n");
    printf("========================================\n");

    char* result = run_all_tests();
    if (result != NULL) {
        printf("\nFAIL: %s\n", result);
        return 1;
    }

    printf("\n========================================\n");
    printf("  All tests passed: %d run\n", tests_run);
    printf("========================================\n");

    return 0;
}
