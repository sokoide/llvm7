#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global token pointer for consume/expect functions
Token* token;

static Token* new_token(TokenKind kind, Token* cur, const char* str, int len) {
    Token* tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->val = 0;
    if (kind == TK_NUM) {
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
Token* tokenize(const char* p) {
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

        if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0 ||
            strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' ||
            *p == ')' || *p == '<' || *p == '>') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        } else if (isdigit(*p)) {
            const char* start = p;
            while (isdigit(*p)) {
                p++;
            }
            cur = new_token(TK_NUM, cur, start, p - start);
            continue;
        }

        else {
            fprintf(stderr, "Lexer error: Invalid character '%c'\n", *p);
            return NULL;
        }
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

/**
 * Free a linked list of tokens
 *
 * @param[in] head Pointer to the head of the token linked list to free
 */
void free_tokens(Token* head) {
    Token* curr = head;
    while (curr != NULL) {
        Token* next = curr->next;
        free(curr);
        curr = next;
    }
}

/**
 * Function to consume a reserved operator token and proceed to the next token
 *
 * @param[in] op The character operator to be consumed
 * @return true if the token was consumed successfully, false otherwise
 */
bool consume(char* op) {
    if (token->kind != TK_RESERVED || (int)strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

/**
 * Checks if the next token matches the expected operator
 * If it doesn't match, prints an error message and exits the program
 * @param[in] op The expected operator string
 */
void expect(char* op) {
    // Try to consume (match) the expected operator
    if (!consume(op)) {
        // If the operator doesn't match, print an error message to stderr
        fprintf(stderr, "Lexer error: Expected '%s'\n", op);
        // Exit the program with an error status
        exit(1);
    }
}

int expect_number() {
    // Check if the current token is a number
    if (token->kind != TK_NUM) {
        // If it's not a number, print an error message to stderr
        fprintf(stderr, "Lexer error: Expected a number\n");
        // Exit the program with an error status
        exit(1);
    }
    // Return the value of the number token
    int val = token->val;
    token = token->next;
    return val;
}