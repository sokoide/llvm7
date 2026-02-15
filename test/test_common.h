#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>

extern int tests_run;

// use do/while to handle the macro as a single statement.
// this is safer because it avoids accidental semicolon insertion in if-else.
#define mu_assert(message, test)                                               \
    do {                                                                       \
        if (!(test))                                                           \
            return message;                                                    \
    } while (0)

#define mu_run_test(test)                                                      \
    do {                                                                       \
        char* message = test();                                                \
        tests_run++;                                                           \
        if (message)                                                           \
            return message;                                                    \
    } while (0)

#endif