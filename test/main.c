#include "generate_test.h"
#include "lexer_test.h"
#include "test_common.h"
#include <stdio.h>

static char* run_all_tests() {
    mu_run_test(test_generate_return_zero, "generate: return 0");
    mu_run_test(test_generate_return_one, "generate: return 1");
    mu_run_test(test_generate_return_42, "generate: return 42");
    mu_run_test(test_generate_return_negative, "generate: return -1");
    mu_run_test(test_generate_return_medium_positive, "generate: return 12345");
    mu_run_test(test_generate_return_max_int, "generate: return INT_MAX");
    mu_run_test(test_generate_return_min_int, "generate: return INT_MIN");
    mu_run_test(test_lexer_tokenize, "lexer: tokenize");
    mu_run_test(test_consume_operator, "lexer: consume operator");
    mu_run_test(test_expect_operator, "lexer: expect operator");
    mu_run_test(test_expect_number, "lexer: expect number");
    return NULL;
}

int main() {
    char* result = run_all_tests();
    if (result != NULL) {
        return 1;
    }

    printf("\n========================================\n");
    printf("  All tests passed: %d run\n", tests_run);
    printf("========================================\n");

    return 0;
}
