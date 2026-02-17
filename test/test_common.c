#include "test_common.h"
#include <stdlib.h>

int tests_run = 0;

void alloc4(int** p, int a, int b, int c, int d) {
    int* mem = malloc(sizeof(int) * 4);
    mem[0] = a;
    mem[1] = b;
    mem[2] = c;
    mem[3] = d;
    *p = mem;
}