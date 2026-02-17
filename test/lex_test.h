#ifndef __LEX_TEST_H__
#define __LEX_TEST_H__

#include "lex.h"

// Test functions
char* test_lex_tokenize();
char* test_consume_operator();
char* test_expect_operator();
char* test_expect_number();
char* test_lex_comments();

#endif
