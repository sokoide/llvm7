#include "generate_test.h"
#include <stdio.h>

int main() {
    printf("========================================\n");
    printf("  LLVM JIT Compiler Test Suite\n");
    printf("  Purpose: Test that code generated from AST executes correctly\n");
    printf("========================================\n");

    // Run all tests
    test_generate_return_zero();
    test_generate_return_one();
    test_generate_return_42();
    test_generate_return_negative();
    test_generate_return_medium_positive();
    test_generate_return_max_int();
    test_generate_return_min_int();

    printf("\n========================================\n");
    printf("  All tests completed successfully\n");
    printf("========================================\n");

    return 0;
}
