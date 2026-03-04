#ifndef __PREPROCESS_TEST_H__
#define __PREPROCESS_TEST_H__

// Test functions
char* test_preprocess_noop();
char* test_preprocess_include();
char* test_preprocess_define();
char* test_preprocess_ifdef();
char* test_preprocess_no_expand_in_literals_or_comments();
char* test_preprocess_macro_scope_is_per_call();
char* test_preprocess_long_define_value();
char* test_preprocess_if_elif_expr();
char* test_preprocess_function_macro_and_undef();
char* test_preprocess_pragma_ignored();
char* test_preprocess_stringification();
char* test_preprocess_token_pasting();

#endif
