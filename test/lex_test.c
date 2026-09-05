#include "lex_test.h"
#include "../src/lex.h"
#include "../src/parse.h"
#include "test_common.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

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
    Token* head = tokenize("a\n/* one\n two */  + 12");
    Token* t = head;

    mu_assert("first token line/col should be 1:1",
              t->line == 1 && t->col == 1);
    t = t->next;
    mu_assert("plus token line/col should be 3:10",
              t->line == 3 && t->col == 10);
    t = t->next;
    mu_assert("number token line/col should be 3:12",
              t->line == 3 && t->col == 12);

    free_tokens(head);
    return NULL;
}

char* test_lex_error_returns_null() {
    mu_assert("invalid character should fail tokenization",
              tokenize("int x; @") == NULL);
    mu_assert("unterminated string should fail tokenization",
              tokenize("int x = \"missing") == NULL);
    mu_assert("unterminated comment should fail tokenization",
              tokenize("int x; /* missing") == NULL);
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

char* test_lex_hex_float() {
    // Test hexadecimal floating-point constant: 0x1p3 = 8.0
    Token* head = tokenize("0x1p3");
    Token* curr = head;

    mu_assert("first token should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("first token should be float", curr->is_float);
    mu_assert("0x1p3 should equal 8.0", curr->fval == 8.0);

    free_tokens(head);
    return NULL;
}

char* test_lex_hex_float_with_fraction() {
    // Test hexadecimal floating-point constant: 0x1.8p1 = 3.0
    Token* head = tokenize("0x1.8p1");
    Token* curr = head;

    mu_assert("first token should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("first token should be float", curr->is_float);
    mu_assert("0x1.8p1 should equal 3.0", curr->fval == 3.0);

    free_tokens(head);
    return NULL;
}

char* test_lex_hex_float_negative_exp() {
    // Test hexadecimal floating-point constant: 0xAp-2 = 2.5
    Token* head = tokenize("0xAp-2");
    Token* curr = head;

    mu_assert("first token should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("first token should be float", curr->is_float);
    mu_assert("0xAp-2 should equal 2.5", curr->fval == 2.5);

    free_tokens(head);
    return NULL;
}

char* test_lex_hex_float_uppercase() {
    // Test uppercase: 0X1P3 = 8.0
    Token* head = tokenize("0X1P3");
    Token* curr = head;

    mu_assert("first token should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("first token should be float", curr->is_float);
    mu_assert("0X1P3 should equal 8.0", curr->fval == 8.0);

    free_tokens(head);
    return NULL;
}

char* test_lex_hex_escape() {
    // '\x41' should be 65 (ASCII 'A')
    Token* head = tokenize("'\\x41'");
    Token* curr = head;
    mu_assert("should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("'\\x41' should be 65", curr->val == 65);
    free_tokens(head);
    return NULL;
}

char* test_lex_hex_escape_two_digit() {
    // '\xff' as signed char is -1
    Token* head = tokenize("'\\xff'");
    Token* curr = head;
    mu_assert("should be TK_NUM", curr->kind == TK_NUM);
    // Check if char is signed (CHAR_MAX == 127) or unsigned (CHAR_MAX == 255)
    if (CHAR_MAX == 127) {
        mu_assert("'\\xff' signed char should be -1", curr->val == -1);
    } else {
        mu_assert("'\\xff' unsigned char should be 255", curr->val == 255);
    }
    free_tokens(head);
    return NULL;
}

char* test_lex_octal_escape() {
    // '\101' should be 65 (octal 101 = 65 = 'A')
    // '\0' should still be 0 (backward compat)
    Token* head = tokenize("'\\101'");
    Token* curr = head;
    mu_assert("should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("'\\101' should be 65", curr->val == 65);
    free_tokens(head);
    return NULL;
}

char* test_lex_octal_escape_zero() {
    // '\0' backward compatibility
    Token* head = tokenize("'\\0'");
    Token* curr = head;
    mu_assert("should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("'\\0' should be 0", curr->val == 0);
    free_tokens(head);
    return NULL;
}

char* test_lex_hex_int() {
    // Test hexadecimal integer: 0x123 = 291
    Token* head = tokenize("0x123");
    Token* curr = head;

    mu_assert("first token should be TK_NUM", curr->kind == TK_NUM);
    mu_assert("first token should not be float", !curr->is_float);
    mu_assert("0x123 should equal 291", curr->uval == 291);

    free_tokens(head);
    return NULL;
}

char* test_lex_leading_dot_float() {
    Token* tokens = tokenize(".5 .25e+2 .125f . ... a.b");
    Token* tok = tokens;
    double values[] = {0.5, 25.0, 0.125};
    int lengths[] = {2, 6, 5};
    for (int i = 0; i < 3; i++) {
        mu_assert("leading dot literal must be a number",
                  tok && tok->kind == TK_NUM);
        mu_assert("leading dot literal must be floating point",
                  tok->is_float);
        mu_assert("floating value must match", tok->fval == values[i]);
        mu_assert("entire literal must be consumed", tok->len == lengths[i]);
        tok = tok->next;
    }
    mu_assert("standalone dot must remain punctuation",
              tok->kind == TK_RESERVED && tok->len == 1);
    tok = tok->next;
    mu_assert("ellipsis must remain one token",
              tok->kind == TK_RESERVED && tok->len == 3);
    tok = tok->next;
    mu_assert("member base must be an identifier", tok->kind == TK_IDENT);
    tok = tok->next;
    mu_assert("member dot must remain punctuation",
              tok->kind == TK_RESERVED && tok->len == 1);
    free_tokens(tokens);
    return NULL;
}

char* test_lex_identifier_keyword_boundaries() {
    Token* tokens = tokenize(
        "int integer int_ int1 _int return return_value short shortfall");
    Token* tok = tokens;
    TokenKind kinds[] = {TK_RESERVED, TK_IDENT, TK_IDENT, TK_IDENT, TK_IDENT,
                         TK_RESERVED, TK_IDENT, TK_RESERVED, TK_IDENT};
    for (int i = 0; i < 9; i++) {
        mu_assert("keyword must match the entire identifier",
                  tok && tok->kind == kinds[i]);
        tok = tok->next;
    }
    mu_assert("all identifiers consumed", tok->kind == TK_EOF);
    free_tokens(tokens);
    return NULL;
}

char* test_lex_incomplete_character_literal() {
    // Heap buffers let AddressSanitizer catch reads past the terminator.
    const char* inputs[] = {"'", "''", "'a", "'\\"};
    for (int i = 0; i < 4; i++) {
        char* input = strdup(inputs[i]);
        Token* tokens = tokenize(input);
        free(input);
        mu_assert("incomplete character literal must fail", tokens == NULL);
    }
    return NULL;
}
