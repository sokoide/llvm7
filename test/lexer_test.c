#include "lexer_test.h"
#include "test_common.h"
#include <stdio.h>

char* test_lexer_tokenize() {
    printf("--- Test: tokenize ---\n");

    Token* token = tokenize("1 + 2 - 3");

    mu_assert("First token should be TK_NUM", token->kind == TK_NUM);
    mu_assert("First token value should be 1", token->val == 1);

    token = token->next;
    mu_assert("Second token should be TK_RESERVED", token->kind == TK_RESERVED);
    mu_assert("Second token should be +", token->str[0] == '+');

    token = token->next;
    mu_assert("Third token should be TK_NUM", token->kind == TK_NUM);
    mu_assert("Third token value should be 2", token->val == 2);

    token = token->next;
    mu_assert("Fourth token should be TK_RESERVED", token->kind == TK_RESERVED);
    mu_assert("Fourth token should be -", token->str[0] == '-');

    token = token->next;
    mu_assert("Fifth token should be TK_NUM", token->kind == TK_NUM);
    mu_assert("Fifth token value should be 3", token->val == 3);

    token = token->next;
    mu_assert("Last token should be TK_EOF", token->kind == TK_EOF);

    return NULL;
}
