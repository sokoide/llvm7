#ifndef __LEXER_H__
#define __LEXER_H__

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
    char* str;
    int len;
};

extern Token* token;

extern Token* tokenize(char* p);

#endif