#include "lexer_test.h"
#include "../src/ast.h"
#include "../src/lexer.h"
#include "test_common.h"
#include <stdio.h>
#include <string.h>

char* test_lexer_tokenize() {
    Token* head = tokenize("1 + 2 - 3");
    Token* curr_token = head;

    // Verify first token
    mu_assert("First token should be TK_NUM",
              curr_token != NULL && curr_token->kind == TK_NUM);
    mu_assert("First token value should be 1",
              curr_token != NULL && curr_token->val == 1);

    curr_token = curr_token->next;
    mu_assert("Second token should be TK_RESERVED",
              curr_token != NULL && curr_token->kind == TK_RESERVED);
    mu_assert("Second token should be +",
              curr_token != NULL &&
                  strncmp(curr_token->str, "+", curr_token->len) == 0);

    curr_token = curr_token->next;
    mu_assert("Third token should be TK_NUM",
              curr_token != NULL && curr_token->kind == TK_NUM);
    mu_assert("Third token value should be 2",
              curr_token != NULL && curr_token->val == 2);

    curr_token = curr_token->next;
    mu_assert("Fourth token should be TK_RESERVED",
              curr_token != NULL && curr_token->kind == TK_RESERVED);
    mu_assert("Fourth token should be -",
              curr_token != NULL &&
                  strncmp(curr_token->str, "-", curr_token->len) == 0);

    curr_token = curr_token->next;
    mu_assert("Fifth token should be TK_NUM",
              curr_token != NULL && curr_token->kind == TK_NUM);
    mu_assert("Fifth token value should be 3",
              curr_token != NULL && curr_token->val == 3);

    curr_token = curr_token->next;
    mu_assert("Last token should be TK_EOF", curr_token->kind == TK_EOF);

    free_tokens(head);
    return NULL;
}

char* test_consume_operator() {
    Context ctx = {0};
    ctx.current_token = tokenize("+ -");

    mu_assert("First token should be +",
              ctx.current_token->kind == TK_RESERVED &&
                  ctx.current_token->str[0] == '+');
    mu_assert("Should consume +", consume(&ctx, "+"));
    mu_assert("Current token should now be -",
              ctx.current_token->kind == TK_RESERVED &&
                  ctx.current_token->str[0] == '-');
    mu_assert("Should not consume *", !consume(&ctx, "*"));
    mu_assert("Should consume -", consume(&ctx, "-"));
    mu_assert("Current token should now be EOF",
              ctx.current_token->kind == TK_EOF);

    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expect_operator() {
    Context ctx = {0};
    ctx.current_token = tokenize("+ 1");

    mu_assert("First token should be +",
              ctx.current_token->kind == TK_RESERVED &&
                  ctx.current_token->str[0] == '+');
    expect(&ctx, "+");
    mu_assert("After expect, token should be number",
              ctx.current_token->kind == TK_NUM);

    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expect_number() {
    Context ctx = {0};
    ctx.current_token = tokenize("42");

    int val = expect_number(&ctx);
    mu_assert("expect_number should return 42", val == 42);
    mu_assert("Current token should now be EOF",
              ctx.current_token->kind == TK_EOF);

    free_tokens(ctx.current_token);
    return NULL;
}
