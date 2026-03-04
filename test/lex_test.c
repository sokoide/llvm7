#include "lex_test.h"
#include "../src/lex.h"
#include "../src/parse.h"
#include "test_common.h"
#include <stdio.h>
#include <string.h>

char* test_lex_tokenize() {
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

char* test_lex_comments() {
    Token* head = tokenize("1 // line comment\n + /* block \n comment */ 2");
    Token* curr = head;

    mu_assert("First token should be 1",
              curr->kind == TK_NUM && curr->val == 1);
    curr = curr->next;
    mu_assert("Second token should be +",
              curr->kind == TK_RESERVED && curr->str[0] == '+');
    curr = curr->next;
    mu_assert("Third token should be 2",
              curr->kind == TK_NUM && curr->val == 2);
    curr = curr->next;
    mu_assert("Last token should be EOF", curr->kind == TK_EOF);

    free_tokens(head);
    return NULL;
}

char* test_lex_get_line_col() {
    const char* src = "aa\nbbb\ncccc";
    int line = 0;
    int col = 0;

    lex_get_line_col(src, src, &line, &col);
    mu_assert("start position should be line 1 col 1", line == 1 && col == 1);

    lex_get_line_col(src, src + 3, &line, &col); // 'b'
    mu_assert("line 2 should start at col 1", line == 2 && col == 1);

    lex_get_line_col(src, src + 8, &line, &col); // second 'c'
    mu_assert("line 3 second char should be col 2", line == 3 && col == 2);

    return NULL;
}

char* test_lex_token_positions() {
    Token* head = tokenize("a\n  + 12");
    Token* t = head;

    mu_assert("first token line/col should be 1:1",
              t->line == 1 && t->col == 1);
    t = t->next;
    mu_assert("plus token line/col should be 2:3", t->line == 2 && t->col == 3);
    t = t->next;
    mu_assert("number token line/col should be 2:5",
              t->line == 2 && t->col == 5);

    free_tokens(head);
    return NULL;
}

char* test_lex_double() {
    Token* head = tokenize("double x = 1.23;");
    Token* curr = head;

    mu_assert("First token should be double",
              curr->kind == TK_RESERVED &&
                  strncmp(curr->str, "double", curr->len) == 0);

    curr = curr->next;
    mu_assert("Second token should be x",
              curr->kind == TK_IDENT &&
                  strncmp(curr->str, "x", curr->len) == 0);

    curr = curr->next;
    mu_assert("Third token should be =",
              curr->kind == TK_RESERVED && curr->str[0] == '=');

    curr = curr->next;
    mu_assert("Fourth token should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("Fourth token fval should be 1.23", curr->fval == 1.23);

    curr = curr->next;
    mu_assert("Fifth token should be ;",
              curr->kind == TK_RESERVED && curr->str[0] == ';');

    free_tokens(head);
    return NULL;
}

char* test_lex_float() {
    Token* head = tokenize("float y = 1.23f;");
    Token* curr = head;

    mu_assert("First token should be float",
              curr->kind == TK_RESERVED &&
                  strncmp(curr->str, "float", curr->len) == 0);

    curr = curr->next;
    mu_assert("Second token should be y",
              curr->kind == TK_IDENT &&
                  strncmp(curr->str, "y", curr->len) == 0);

    curr = curr->next;
    mu_assert("Third token should be =",
              curr->kind == TK_RESERVED && curr->str[0] == '=');

    curr = curr->next;
    mu_assert("Fourth token should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("Fourth token should be float", curr->is_float);
    mu_assert("Fourth token fval should be 1.23", curr->fval == 1.23);

    curr = curr->next;
    mu_assert("Fifth token should be ;",
              curr->kind == TK_RESERVED && curr->str[0] == ';');

    free_tokens(head);
    return NULL;
}

char* test_lex_bitwise() {
    Token* head = tokenize("& | ^ ~ << >> &= |= ^= <<= >>=");
    Token* curr = head;

    mu_assert("token should be &",
              curr->kind == TK_RESERVED && strncmp(curr->str, "&", 1) == 0);
    curr = curr->next;
    mu_assert("token should be |",
              curr->kind == TK_RESERVED && strncmp(curr->str, "|", 1) == 0);
    curr = curr->next;
    mu_assert("token should be ^",
              curr->kind == TK_RESERVED && strncmp(curr->str, "^", 1) == 0);
    curr = curr->next;
    mu_assert("token should be ~",
              curr->kind == TK_RESERVED && strncmp(curr->str, "~", 1) == 0);
    curr = curr->next;
    mu_assert("token should be <<",
              curr->kind == TK_RESERVED && strncmp(curr->str, "<<", 2) == 0);
    curr = curr->next;
    mu_assert("token should be >>",
              curr->kind == TK_RESERVED && strncmp(curr->str, ">>", 2) == 0);
    curr = curr->next;
    mu_assert("token should be &=",
              curr->kind == TK_RESERVED && strncmp(curr->str, "&=", 2) == 0);
    curr = curr->next;
    mu_assert("token should be |=",
              curr->kind == TK_RESERVED && strncmp(curr->str, "|=", 2) == 0);
    curr = curr->next;
    mu_assert("token should be ^=",
              curr->kind == TK_RESERVED && strncmp(curr->str, "^=", 2) == 0);
    curr = curr->next;
    mu_assert("token should be <<=",
              curr->kind == TK_RESERVED && strncmp(curr->str, "<<=", 3) == 0);
    curr = curr->next;
    mu_assert("token should be >>=",
              curr->kind == TK_RESERVED && strncmp(curr->str, ">>=", 3) == 0);
    curr = curr->next;

    free_tokens(head);
    return NULL;
}

char* test_lex_no_unsigned_suffix_on_float() {
    Token* head = tokenize("1.0u");
    Token* curr = head;

    mu_assert("first token should be float TK_NUM", curr->kind == TK_NUM);
    mu_assert("first token should be float", curr->is_float);
    mu_assert("float token should not be marked unsigned", !curr->is_unsigned);
    curr = curr->next;
    mu_assert("second token should be identifier 'u'",
              curr->kind == TK_IDENT && curr->len == 1 && curr->str[0] == 'u');

    free_tokens(head);
    return NULL;
}

char* test_lex_large_unsigned_literal() {
    Token* head = tokenize("4294967295u;");
    Token* curr = head;

    mu_assert("first token should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("first token should be unsigned", curr->is_unsigned);
    mu_assert("uval should keep full unsigned literal",
              curr->uval == 4294967295ULL);

    free_tokens(head);
    return NULL;
}
