#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

#include <stdio.h>
#include <stdlib.h>

extern int tests_run;

// Test helper function defined in test_common.c
extern void alloc4(int** p, int a, int b, int c, int d);

// use do/while to handle the macro as a single statement.
// this is safer because it avoids accidental semicolon insertion in if-else.
#define mu_assert(message, test)                                               \
    do {                                                                       \
        if (!(test))                                                           \
            return message;                                                    \
    } while (0)

#define mu_run_test(test, name)                                                \
    do {                                                                       \
        printf("--- %s ---\n", name);                                          \
        char* message = test();                                                \
        tests_run++;                                                           \
        if (message) {                                                         \
            printf("FAIL: %s\n", message);                                     \
            return message;                                                    \
        } else {                                                               \
            printf("PASS\n");                                                  \
        }                                                                      \
    } while (0)

#endif
