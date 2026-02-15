#include "lexer_test.h"
#include "test_common.h"
#include <stdio.h>

char* test_lexer_tokenize() {
    // Reset global token before calling tokenize
    token = NULL;
    Token* curr_token = tokenize("1 + 2 - 3");

    // Verify first token
    mu_assert("First token should be TK_NUM", curr_token != NULL && curr_token->kind == TK_NUM);
    mu_assert("First token value should be 1", curr_token != NULL && curr_token->val == 1);

    curr_token = curr_token->next;
    mu_assert("Second token should be TK_RESERVED", curr_token != NULL && curr_token->kind == TK_RESERVED);
    mu_assert("Second token should be +", curr_token != NULL && curr_token->str[0] == '+');

    curr_token = curr_token->next;
    mu_assert("Third token should be TK_NUM", curr_token != NULL && curr_token->kind == TK_NUM);
    mu_assert("Third token value should be 2", curr_token != NULL && curr_token->val == 2);

    curr_token = curr_token->next;
    mu_assert("Fourth token should be TK_RESERVED", curr_token != NULL && curr_token->kind == TK_RESERVED);
    mu_assert("Fourth token should be -", curr_token != NULL && curr_token->str[0] == '-');

    curr_token = curr_token->next;
    mu_assert("Fifth token should be TK_NUM", curr_token != NULL && curr_token->kind == TK_NUM);
    mu_assert("Fifth token value should be 3", curr_token != NULL && curr_token->val == 3);

    curr_token = curr_token->next;
    mu_assert("Last token should be TK_EOF", curr_token->kind == TK_EOF);

    return NULL;
}

char* test_consume_operator() {
    token = NULL;
    Token* curr_token = tokenize("+ -");
    token = curr_token;

    mu_assert("First token should be +", token->kind == TK_RESERVED && token->str[0] == '+');
    mu_assert("Should consume +", consume('+'));
    mu_assert("Current token should now be -", token->kind == TK_RESERVED && token->str[0] == '-');
    mu_assert("Should not consume *", !consume('*'));
    mu_assert("Should consume -", consume('-'));
    mu_assert("Current token should now be EOF", token->kind == TK_EOF);

    return NULL;
}

char* test_expect_operator() {
    token = NULL;
    Token* curr_token = tokenize("+ 1");
    token = curr_token;

    mu_assert("First token should be +", token->kind == TK_RESERVED && token->str[0] == '+');
    expect('+');
    mu_assert("After expect, token should be number", token->kind == TK_NUM);

    return NULL;
}

char* test_expect_number() {
    token = NULL;
    Token* curr_token = tokenize("42");
    token = curr_token;

    int val = expect_number();
    mu_assert("expect_number should return 42", val == 42);
    mu_assert("Current token should now be EOF", token->kind == TK_EOF);

    return NULL;
}
