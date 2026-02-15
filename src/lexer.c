#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token* token;

static Token* new_token(TokenKind kind, Token* cur, char* str) {
    Token* tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->val = 0;
    if (kind == TK_NUM) {
        tok->val = strtol(str, NULL, 10);
    }
    tok->str = str;
    tok->len = strlen(str);
    cur->next = tok;
    return tok;
}

/**
 * Tokenize a string into a linked list of tokens
 *
 * @param[in] p The input string to be tokenized
 * @return Pointer to the head of the token linked list
 */
Token* tokenize(char* p) {
    // Initialize head and tail pointers for the token linked list
    Token head;
    head.next = NULL;
    Token* cur = &head;

    // Iterate through the input string until null terminator
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p);
            p++;
            continue;
        } else if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            p++;
            continue;
        }

        else {
            fprintf(stderr, "Lexer error: Invalid character '%c'\n", *p);
            return NULL;
        }
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}
