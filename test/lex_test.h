#ifndef __LEX_TEST_H__
#define __LEX_TEST_H__

#include "lex.h"

// Test functions
char* test_lex_tokenize();
char* test_consume_operator();
char* test_expect_operator();
char* test_expect_number();
char* test_lex_comments();
char* test_lex_get_line_col();
char* test_lex_token_positions();
char* test_lex_double();
char* test_lex_float();
char* test_lex_bitwise();
char* test_lex_no_unsigned_suffix_on_float();
char* test_lex_large_unsigned_literal();
char* test_lex_hex_float();
char* test_lex_hex_float_with_fraction();
char* test_lex_hex_float_negative_exp();
char* test_lex_hex_float_uppercase();
char* test_lex_hex_int();
char* test_lex_hex_escape();
char* test_lex_hex_escape_two_digit();
char* test_lex_octal_escape();
char* test_lex_octal_escape_zero();

#endif
