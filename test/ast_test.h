#ifndef __AST_TEST_H__
#define __AST_TEST_H__

#include "../src/ast.h"

// Test functions
char* test_new_node_num();
char* test_new_node();
char* test_primary_num();
char* test_primary_paren();
char* test_mul_single_num();
char* test_mul_multiply();
char* test_mul_divide();
char* test_mul_chain();
char* test_expr_single_num();
char* test_expr_add();
char* test_expr_subtract();
char* test_expr_add_sub_chain();
char* test_expr_precedence();
char* test_expr_complex_precedence();

#endif
