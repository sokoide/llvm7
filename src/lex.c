#include "lex.h"
#include "parse.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_alnum(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') || c == '_';
}

static char decode_escape_char(const char **pp) {
    const char *p = *pp;
    if (*p == 'n') {
        *pp = p + 1;
        return '\n';
    }
    if (*p == 'r') {
        *pp = p + 1;
        return '\r';
    }
    if (*p == 't') {
        *pp = p + 1;
        return '\t';
    }
    if (*p == '0') {
        *pp = p + 1;
        return '\0';
    }
    if (*p == '\\' || *p == '"' || *p == '\'') {
        *pp = p + 1;
        return *p;
    }
    if (*p == '\0') {
        return '\0';
    }
    *pp = p + 1;
    return *p;
}

// Keyword table (selfhost-compatible: no anonymous struct)
char *kw_str[28] = {
    "return", "if",     "else",    "while",   "for",      "int",      "char",
    "void",   "sizeof", "struct",  "typedef", "enum",     "static",   "extern",
    "const",  "long",   "bool",    "size_t",  "NULL",     "true",     "false",
    "switch", "case",   "default", "break",   "continue", "unsigned", "signed"};
int kw_len[28] = {6, 2, 4, 5, 3, 3, 4, 4, 6, 6, 7, 4, 6, 6,
                  5, 4, 4, 6, 4, 4, 5, 6, 4, 7, 5, 8, 8, 6};
int NUM_KEYWORDS = 28;

char *three_char_ops[1] = {"..."};
int NUM_THREE_CHAR_OPS = 1;

static char *two_char_ops[] = {"==", "!=", "<=", ">=", "&&", "||", "->",
                               "++", "--", "+=", "-=", "*=", "/=", "<<"};

static const int NUM_TWO_CHAR_OPS = sizeof(two_char_ops) /
                                   sizeof(two_char_ops[0]);

Token *new_token(TokenKind kind, Token *cur, const char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->val = 0;
    if (kind == TK_NUM && len > 0) {
        tok->val = strtol(str, NULL, 10);
    }
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

/**
 * Tokenize a string into a linked list of tokens
 *
 * @param[in] p The input string to be tokenized
 * @return Pointer to the head of the token linked list
 */
Token *tokenize(const char *p) {
    // Initialize head and tail pointers for the token linked list
    Token head;
    head.next = NULL;
    Token *cur = &head;

    // Iterate through the input string until null terminator
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Skip line comments
        if (strncmp(p, "//", 2) == 0) {
            p += 2;
            while (*p && *p != '\n')
                p++;
            continue;
        }

        // Skip block comments
        if (strncmp(p, "/*", 2) == 0) {
            const char *q = strstr(p + 2, "*/");
            if (!q) {
                fprintf(stderr, "lex error: unterminated block comment\n");
                return NULL;
            }
            p = q + 2;
            continue;
        }

        // Check for keywords
        bool keyword_matched = false;
        for (int i = 0; i < NUM_KEYWORDS; i++) {
            if (strncmp(p, kw_str[i], kw_len[i]) == 0 &&
                !is_alnum(p[kw_len[i]])) {
                cur = new_token(TK_RESERVED, cur, p, kw_len[i]);
                p += kw_len[i];
                keyword_matched = true;
                break;
            }
        }
        if (keyword_matched) {
            continue;
        }

        // Check for three-character operators (ellipsis)
        bool three_char_matched = false;
        for (int i = 0; i < NUM_THREE_CHAR_OPS; i++) {
            if (strncmp(p, three_char_ops[i], 3) == 0) {
                cur = new_token(TK_RESERVED, cur, p, 3);
                p += 3;
                three_char_matched = true;
                break;
            }
        }
        if (three_char_matched) {
            continue;
        }

        // Check for two-character operators
        bool two_char_matched = false;
        for (int i = 0; i < NUM_TWO_CHAR_OPS; i++) {
            if (strncmp(p, two_char_ops[i], 2) == 0) {
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
                two_char_matched = true;
                break;
            }
        }
        if (two_char_matched) {
            continue;
        }

        // Check for single-character operators and delimiters
        char *single_char_ops = "+-*/()<>;={},&|[].!:=?%.\0";
        if (strchr(single_char_ops, *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // String literal
        if (*p == '"') {
            p++; // skip opening quote
            size_t cap = strlen(p) + 1;
            char *decoded = calloc(cap, 1);
            size_t len = 0;
            while (*p && *p != '"') {
                if (*p == '\\') {
                    p++;
                    decoded[len++] = decode_escape_char(&p);
                    continue;
                }
                decoded[len++] = *p;
                p++;
            }
            if (*p != '"') {
                fprintf(stderr, "lex error: unterminated string literal\n");
                return NULL;
            }
            cur = new_token(TK_STR, cur, decoded, (int)len);
            p++; // skip closing quote
            continue;
        }

        // Character literal
        if (*p == '\'') {
            p++; // skip '
            int val;
            if (*p == '\\') {
                p++;
                val = decode_escape_char(&p);
            } else {
                val = *p++;
            }

            if (*p != '\'') {
                fprintf(stderr, "lex error: unterminated character literal\n");
                return NULL;
            }
            p++; // skip closing '
            cur = new_token(TK_NUM, cur, p - 2, 1);
            cur->val = val;
            cur->len =
                0; // Still use 0 to avoid consume() matching, but wait...
            // Let's use actual length 1 and pointed at the char but it's
            // complex. Actually, the best way is to not use len=0 if it's a
            // valid token. For TK_NUM, expect_number() uses cur->val. So
            // kind=TK_NUM, len=0 is fine IF nobody uses len to match it. Wait,
            // the previous logic was creating a SECOND token!
            continue;
        }

        // Number
        if (isdigit(*p)) {
            const char *start = p;
            while (isdigit(*p)) {
                p++;
            }
            cur = new_token(TK_NUM, cur, start, p - start);
            continue;
        }

        // Identifier
        if (('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z') || *p == '_') {
            const char *start = p;
            while (('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z') ||
                   ('0' <= *p && *p <= '9') || *p == '_') {
                p++;
            }
            cur = new_token(TK_IDENT, cur, start, p - start);
            continue;
        }

        // Error
        fprintf(stderr, "lex error: Invalid character '%c'\n", *p);
        return NULL;
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

/**
 * Free a linked list of tokens
 *
 * @param[in] head Pointer to the head of the token linked list to free
 */
void free_tokens(Token *head) {
    Token *curr = head;
    while (curr != NULL) {
        Token *next = curr->next;
        free(curr);
        curr = next;
    }
}

/**
 * Function to consume a reserved operator token and proceed to the next token
 *
 * @param[in] ctx Context containing current token
 * @param[in] op The character operator to be consumed
 * @return true if the token was consumed successfully, false otherwise
 */
bool consume(Context *ctx, char *op) {
    if (ctx->current_token->kind != TK_RESERVED ||
        (int)strlen(op) != ctx->current_token->len ||
        memcmp(ctx->current_token->str, op, ctx->current_token->len)) {
        return false;
    }
    ctx->current_token = ctx->current_token->next;
    return true;
}

/**
 * Function to consume and return an identifier token
 *
 * @param[in] ctx Context containing current token
 * @return Pointer to the identifier token if the current token is an
 * identifier, NULL otherwise
 */
Token *consume_ident(Context *ctx) {
    // Check if the current token is an identifier
    if (ctx->current_token->kind != TK_IDENT) {
        return NULL;
    }
    // Store current token and advance to the next token
    Token *t = ctx->current_token;
    ctx->current_token = ctx->current_token->next;
    return t;
}

/**
 * Checks if the next token matches the expected operator
 * If it doesn't match, prints an error message and exits the program
 * @param[in] ctx Context containing current token
 * @param[in] op The expected operator string
 */
void expect(Context *ctx, char *op) {
    // Try to consume (match) the expected operator
    if (!consume(ctx, op)) {
        // If the operator doesn't match, print an error message to stderr
        fprintf(stderr, "lex error: Expected '%s'\n", op);
        // Context: fprintf(stderr, "Context: '%.20s'\n",
        // ctx->current_token->str); Exit the program with an error status
        exit(1);
    }
}

int expect_number(Context *ctx) {
    // Check if the current token is a number
    if (ctx->current_token->kind != TK_NUM) {
        // If it's not a number, print an error message to stderr
        fprintf(stderr, "lex error: Expected a number\n");
        // Exit the program with an error status
        exit(1);
    }
    // Return the value of the number token
    int val = ctx->current_token->val;
    ctx->current_token = ctx->current_token->next;
    return val;
}

bool at_eof(Context *ctx) { return ctx->current_token->kind == TK_EOF; }
