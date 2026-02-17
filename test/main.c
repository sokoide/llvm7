#include "codegen_test.h"
#include "lex_test.h"
#include "parse_test.h"
#include "test_common.h"
#include <stdio.h>

static char* run_all_tests() {
    mu_run_test(test_generate_return_42, "codegen: return 42");
    mu_run_test(test_generate_return_negative, "codegen: return -1");
    mu_run_test(test_generate_return_max_int, "codegen: return INT_MAX");
    mu_run_test(test_generate_return_min_int, "codegen: return INT_MIN");
    mu_run_test(test_generate_add, "codegen: add");
    mu_run_test(test_generate_sub, "codegen: sub");
    mu_run_test(test_generate_mul, "codegen: mul");
    mu_run_test(test_generate_div, "codegen: div");
    mu_run_test(test_generate_precedence, "codegen: precedence");
    mu_run_test(test_generate_parentheses, "codegen: parentheses");
    mu_run_test(test_generate_complex, "codegen: complex");
    mu_run_test(test_generate_lt_true, "codegen: lt true");
    mu_run_test(test_generate_lt_false, "codegen: lt false");
    mu_run_test(test_generate_eq_true, "codegen: eq true");
    mu_run_test(test_generate_ne_false, "codegen: ne false");
    mu_run_test(test_generate_main_required, "codegen: main required");
    mu_run_test(test_generate_pointer_example, "codegen: pointer example");
    mu_run_test(test_generate_pointer_arithmetic,
                "codegen: pointer arithmetic");
    mu_run_test(test_generate_pointer_arithmetic_sub,
                "codegen: pointer arithmetic sub");
    mu_run_test(test_generate_sizeof, "codegen: sizeof");
    mu_run_test(test_generate_sizeof_expr, "codegen: sizeof expr");
    mu_run_test(test_generate_array, "codegen: array");
    mu_run_test(test_generate_array_subscript, "codegen: array subscript");
    mu_run_test(test_generate_array_subscript_reversed,
                "codegen: array subscript reversed");
    mu_run_test(test_generate_global_var, "codegen: global var");
    mu_run_test(test_generate_global_ptr_var, "codegen: global ptr var");
    mu_run_test(test_generate_global_array, "codegen: global array");
    mu_run_test(test_generate_global_array_subscript,
                "codegen: global array subscript");
    mu_run_test(test_generate_global_ptr_return_func,
                "codegen: global ptr return func");
    mu_run_test(test_generate_char_array, "codegen: char array");
    mu_run_test(test_generate_string_literal, "codegen: string literal");
    mu_run_test(test_lex_tokenize, "lex: tokenize");

    mu_run_test(test_consume_operator, "lex: consume operator");
    mu_run_test(test_expect_operator, "lex: expect operator");
    mu_run_test(test_expect_number, "lex: expect number");
    mu_run_test(test_new_node_num, "parse: new_node_num");
    mu_run_test(test_new_node, "parse: new_node");
    mu_run_test(test_unary_num, "parse: unary num");
    mu_run_test(test_unary_plus, "parse: unary plus");
    mu_run_test(test_unary_minus, "parse: unary minus");
    mu_run_test(test_unary_plus_num, "parse: unary plus num");
    mu_run_test(test_unary_minus_mul, "parse: unary minus mul");
    mu_run_test(test_primary_num, "parse: primary number");
    mu_run_test(test_primary_paren, "parse: primary paren");
    mu_run_test(test_mul_single_num, "parse: mul single number");
    mu_run_test(test_mul_multiply, "parse: mul multiply");
    mu_run_test(test_mul_divide, "parse: mul divide");
    mu_run_test(test_mul_chain, "parse: mul chain");
    mu_run_test(test_expr_single_num, "parse: expr single number");
    mu_run_test(test_expr_add, "parse: expr add");
    mu_run_test(test_expr_subtract, "parse: expr subtract");
    mu_run_test(test_expr_add_sub_chain, "parse: expr add/sub chain");
    mu_run_test(test_expr_precedence, "parse: expr precedence");
    mu_run_test(test_expr_complex_precedence, "parse: expr complex precedence");
    mu_run_test(test_relational_lt, "parse: relational lt");
    mu_run_test(test_relational_le, "parse: relational le");
    mu_run_test(test_relational_gt, "parse: relational gt");
    mu_run_test(test_relational_ge, "parse: relational ge");
    mu_run_test(test_equality_eq, "parse: equality eq");
    mu_run_test(test_equality_ne, "parse: equality ne");
    mu_run_test(test_expr_combined_precedence,
                "parse: expr combined precedence");
    mu_run_test(test_new_node_ident, "parse: new_node_ident");
    mu_run_test(test_new_node_ident_abc, "parse: new_node_ident_abc");
    mu_run_test(test_primary_ident, "parse: primary ident");
    mu_run_test(test_assign, "parse: assign");
    mu_run_test(test_assign_chain, "parse: assign chain");
    mu_run_test(test_stmt, "parse: stmt");
    mu_run_test(test_stmt_assign, "parse: stmt assign");
    mu_run_test(test_stmt_return_num, "parse: stmt return num");
    mu_run_test(test_stmt_return_ident, "parse: stmt return ident");
    mu_run_test(test_stmt_return_expr, "parse: stmt return expr");
    mu_run_test(test_stmt_if, "parse: stmt if");
    mu_run_test(test_stmt_if_else, "parse: stmt if else");
    mu_run_test(test_stmt_if_with_block, "parse: stmt if with block");
    mu_run_test(test_stmt_if_complex_cond, "parse: stmt if complex cond");
    mu_run_test(test_stmt_while, "parse: stmt while");
    mu_run_test(test_stmt_while_complex_cond, "parse: stmt while complex cond");
    mu_run_test(test_stmt_for, "parse: stmt for");
    mu_run_test(test_stmt_for_no_init, "parse: stmt for no init");
    mu_run_test(test_stmt_call, "parse: stmt call");
    mu_run_test(test_stmt_call_with_args, "parse: stmt call with args");
    mu_run_test(test_program_single_stmt, "parse: program single stmt");
    mu_run_test(test_program_multiple_stmts, "parse: program multiple stmts");
    mu_run_test(test_program_assign_stmts, "parse: program assign stmts");
    mu_run_test(test_function_simple, "parse: function simple");
    mu_run_test(test_function_multiple_stmts, "parse: function multiple stmts");
    mu_run_test(test_program_single_function, "parse: program single function");
    mu_run_test(test_program_multiple_functions,
                "parse: program multiple functions");
    mu_run_test(test_function_with_block, "parse: function with block");
    mu_run_test(test_function_with_params, "parse: function with params");
    mu_run_test(test_function_with_single_param,
                "parse: function with single param");
    mu_run_test(test_unary_deref, "parse: unary deref");
    mu_run_test(test_unary_addr, "parse: unary addr");
    mu_run_test(test_unary_deref_complex, "parse: unary deref complex");
    mu_run_test(test_expr_with_deref, "parse: expr with deref");
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
