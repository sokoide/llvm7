#ifndef __LEXER_H__
#define __LEXER_H__

#include <stdbool.h>
typedef enum {
    TK_RESERVED,
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token* next;
    int val;
    const char* str;
    int len;
};

extern Token* token;

extern Token* tokenize(const char* p);
extern void free_tokens(Token* head);
extern bool consume(char op);
extern void expect(char op);
extern int expect_number();

#endif