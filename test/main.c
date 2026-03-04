#include "codegen_test.h"
#include "file_test.h"
#include "lex_test.h"
#include "parse_test.h"
#include "preprocess_test.h"
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
    mu_run_test(test_generate_struct, "codegen: struct");
    mu_run_test(test_generate_struct_simple, "codegen: struct simple");
    mu_run_test(test_generate_struct_assign, "codegen: struct assign");
    mu_run_test(test_generate_char_simple, "codegen: char simple");
    mu_run_test(test_generate_comments, "codegen: comments");
    mu_run_test(test_generate_sizeof_types, "codegen: sizeof types");
    mu_run_test(test_generate_logic, "codegen: logic");
    mu_run_test(test_generate_char_literal, "codegen: char literal");
    mu_run_test(test_generate_arrow, "codegen: arrow operator");
    mu_run_test(test_generate_typedef, "codegen: typedef");
    mu_run_test(test_generate_enum, "codegen: enum");
    mu_run_test(test_generate_builtin_const, "codegen: builtin constants");
    mu_run_test(test_generate_switch, "codegen: switch");
    mu_run_test(test_generate_switch_distinct_case_values,
                "codegen: switch distinct case values");
    mu_run_test(test_generate_switch_case_after_return_case,
                "codegen: switch case after return case");
    mu_run_test(test_generate_inc_dec, "codegen: inc dec");
    mu_run_test(test_generate_proto_and_init, "codegen: proto and init");
    mu_run_test(test_generate_double_add, "codegen: double add");
    mu_run_test(test_generate_double_sub, "codegen: double sub");
    mu_run_test(test_generate_double_mul, "codegen: double mul");
    mu_run_test(test_generate_double_div, "codegen: double div");
    mu_run_test(test_generate_double_compare, "codegen: double compare");
    mu_run_test(test_generate_float_add, "codegen: float add");
    mu_run_test(test_generate_float_sub, "codegen: float sub");
    mu_run_test(test_generate_float_to_double, "codegen: float to double");
    mu_run_test(test_generate_double_to_float, "codegen: double to float");
    mu_run_test(test_generate_double_from_int, "codegen: double from int");
    mu_run_test(test_generate_int_from_double, "codegen: int from double");
    mu_run_test(test_generate_do_while, "codegen: do-while");
    mu_run_test(test_generate_ternary_float, "codegen: ternary float");
    mu_run_test(test_generate_ternary_mixed, "codegen: ternary mixed");
    mu_run_test(test_generate_ternary_int_double_common_type,
                "codegen: ternary int/double common type");
    mu_run_test(test_generate_unsigned, "codegen: unsigned operations");
    mu_run_test(test_generate_bitwise, "codegen: bitwise operations");
    mu_run_test(test_generate_compound_bitwise,
                "codegen: compound bitwise operations");
    mu_run_test(test_generate_union_overlap, "codegen: union overlap");
    mu_run_test(test_generate_bitfield_access, "codegen: bitfield access");
    mu_run_test(test_generate_goto_label, "codegen: goto label");
    mu_run_test(test_generate_designated_initializer_array,
                "codegen: designated initializer array");
    mu_run_test(test_generate_designated_initializer_struct,
                "codegen: designated initializer struct");
    mu_run_test(test_read_file_success, "file: read_file success");
    mu_run_test(test_read_file_not_found, "file: read_file not found");
    mu_run_test(test_lex_tokenize, "lex: tokenize");

    mu_run_test(test_consume_operator, "lex: consume operator");
    mu_run_test(test_expect_operator, "lex: expect operator");
    mu_run_test(test_expect_number, "lex: expect number");
    mu_run_test(test_lex_comments, "lex: comments");
    mu_run_test(test_lex_get_line_col, "lex: get line col");
    mu_run_test(test_lex_token_positions, "lex: token positions");
    mu_run_test(test_lex_double, "lex: double and floats");
    mu_run_test(test_lex_float, "lex: float with suffix");
    mu_run_test(test_lex_bitwise, "lex: bitwise operators");
    mu_run_test(test_lex_no_unsigned_suffix_on_float,
                "lex: reject float unsigned suffix");
    mu_run_test(test_lex_large_unsigned_literal,
                "lex: keep large unsigned literal");
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
    mu_run_test(test_parse_add_assign_does_not_share_lhs_node,
                "parse: compound assign no shared lhs");
    mu_run_test(test_global_ptr_init_not_treated_as_array,
                "parse: global ptr init not array");
    mu_run_test(test_scope_depth_is_context_local,
                "parse: scope depth is context local");
    mu_run_test(test_parse_double, "parse: double declarations");
    mu_run_test(test_parse_float, "parse: float declarations");
    mu_run_test(test_parse_do_while, "parse: do-while");
    mu_run_test(test_parse_uac, "parse: uac");
    mu_run_test(test_parse_bitwise, "parse: bitwise");
    mu_run_test(test_parse_shift, "parse: shift");
    mu_run_test(test_parse_compound_bitwise, "parse: compound bitwise assign");
    mu_run_test(test_parse_union_decl, "parse: union declaration");
    mu_run_test(test_parse_bitfield_decl, "parse: bitfield declaration");
    mu_run_test(test_parse_goto_label, "parse: goto label");
    mu_run_test(test_parse_designated_initializer_array,
                "parse: designated initializer array");
    mu_run_test(test_parse_designated_initializer_struct,
                "parse: designated initializer struct");
    mu_run_test(test_preprocess_noop, "preprocess: no-op");
    mu_run_test(test_preprocess_include, "preprocess: include");
    mu_run_test(test_preprocess_define, "preprocess: define");
    mu_run_test(test_preprocess_ifdef, "preprocess: ifdef");
    mu_run_test(test_preprocess_no_expand_in_literals_or_comments,
                "preprocess: no expand in literals/comments");
    mu_run_test(test_preprocess_macro_scope_is_per_call,
                "preprocess: macro scope per call");
    mu_run_test(test_preprocess_long_define_value,
                "preprocess: long define value");
    mu_run_test(test_preprocess_if_elif_expr, "preprocess: if/elif expression");
    mu_run_test(test_preprocess_function_macro_and_undef,
                "preprocess: function macro and undef");
    mu_run_test(test_preprocess_pragma_ignored, "preprocess: pragma ignored");
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
