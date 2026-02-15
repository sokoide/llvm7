#include "ast_test.h"
#include "generate_test.h"
#include "lexer_test.h"
#include "test_common.h"
#include <stdio.h>

static char* run_all_tests() {
    mu_run_test(test_generate_return_42, "generate: return 42");
    mu_run_test(test_generate_return_negative, "generate: return -1");
    mu_run_test(test_generate_return_max_int, "generate: return INT_MAX");
    mu_run_test(test_generate_return_min_int, "generate: return INT_MIN");
    mu_run_test(test_generate_add, "generate: add");
    mu_run_test(test_generate_sub, "generate: sub");
    mu_run_test(test_generate_mul, "generate: mul");
    mu_run_test(test_generate_div, "generate: div");
    mu_run_test(test_generate_precedence, "generate: precedence");
    mu_run_test(test_generate_parentheses, "generate: parentheses");
    mu_run_test(test_generate_complex, "generate: complex");
    mu_run_test(test_generate_lt_true, "generate: lt true");
    mu_run_test(test_generate_lt_false, "generate: lt false");
    mu_run_test(test_generate_eq_true, "generate: eq true");
    mu_run_test(test_generate_ne_false, "generate: ne false");
    mu_run_test(test_lexer_tokenize, "lexer: tokenize");
    mu_run_test(test_consume_operator, "lexer: consume operator");
    mu_run_test(test_expect_operator, "lexer: expect operator");
    mu_run_test(test_expect_number, "lexer: expect number");
    mu_run_test(test_new_node_num, "ast: new_node_num");
    mu_run_test(test_new_node, "ast: new_node");
    mu_run_test(test_unary_num, "ast: unary num");
    mu_run_test(test_unary_plus, "ast: unary plus");
    mu_run_test(test_unary_minus, "ast: unary minus");
    mu_run_test(test_unary_plus_num, "ast: unary plus num");
    mu_run_test(test_unary_minus_mul, "ast: unary minus mul");
    mu_run_test(test_primary_num, "ast: primary number");
    mu_run_test(test_primary_paren, "ast: primary paren");
    mu_run_test(test_mul_single_num, "ast: mul single number");
    mu_run_test(test_mul_multiply, "ast: mul multiply");
    mu_run_test(test_mul_divide, "ast: mul divide");
    mu_run_test(test_mul_chain, "ast: mul chain");
    mu_run_test(test_expr_single_num, "ast: expr single number");
    mu_run_test(test_expr_add, "ast: expr add");
    mu_run_test(test_expr_subtract, "ast: expr subtract");
    mu_run_test(test_expr_add_sub_chain, "ast: expr add/sub chain");
    mu_run_test(test_expr_precedence, "ast: expr precedence");
    mu_run_test(test_expr_complex_precedence, "ast: expr complex precedence");
    mu_run_test(test_relational_lt, "ast: relational lt");
    mu_run_test(test_relational_le, "ast: relational le");
    mu_run_test(test_relational_gt, "ast: relational gt");
    mu_run_test(test_relational_ge, "ast: relational ge");
    mu_run_test(test_equality_eq, "ast: equality eq");
    mu_run_test(test_equality_ne, "ast: equality ne");
    mu_run_test(test_expr_combined_precedence, "ast: expr combined precedence");
    mu_run_test(test_new_node_ident, "ast: new_node_ident");
    mu_run_test(test_new_node_ident_b, "ast: new_node_ident_b");
    mu_run_test(test_primary_ident, "ast: primary ident");
    mu_run_test(test_assign, "ast: assign");
    mu_run_test(test_assign_chain, "ast: assign chain");
    mu_run_test(test_stmt, "ast: stmt");
    mu_run_test(test_stmt_assign, "ast: stmt assign");
    mu_run_test(test_program_single_stmt, "ast: program single stmt");
    mu_run_test(test_program_multiple_stmts, "ast: program multiple stmts");
    mu_run_test(test_program_assign_stmts, "ast: program assign stmts");
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
