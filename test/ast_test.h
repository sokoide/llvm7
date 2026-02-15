#ifndef __AST_TEST_H__
#define __AST_TEST_H__

#include "../src/ast.h"

// Test functions
char* test_new_node_num();
char* test_new_node();
char* test_unary_num();
char* test_unary_plus();
char* test_unary_minus();
char* test_unary_plus_num();
char* test_unary_minus_mul();
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
char* test_relational_lt();
char* test_relational_le();
char* test_relational_gt();
char* test_relational_ge();
char* test_equality_eq();
char* test_equality_ne();
char* test_expr_combined_precedence();
char* test_new_node_ident();
char* test_new_node_ident_b();
char* test_primary_ident();
char* test_assign();
char* test_assign_chain();
char* test_stmt();
char* test_stmt_assign();
char* test_program_single_stmt();
char* test_program_multiple_stmts();
char* test_program_assign_stmts();

#endif
