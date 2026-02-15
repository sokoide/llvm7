#ifndef __LEXER_H__
#define __LEXER_H__

#include <stdbool.h>
typedef enum {
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
    TK_EOF,
} TokenKind;

struct Token {
    TokenKind kind;
    struct Token* next;
    int val;
    const char* str;
    int len;
};

// Typedef - guarded to avoid redefinition warnings
#ifndef TOKEN_TYPEDEF_DEFINED
#define TOKEN_TYPEDEF_DEFINED
typedef struct Token Token;
#endif

// Forward declaration - full definition in ast.h
#ifndef CONTEXT_TYPEDEF_DEFINED
#define CONTEXT_TYPEDEF_DEFINED
typedef struct Context Context;
#endif

extern Token* tokenize(const char* p);
extern void free_tokens(Token* head);
extern bool consume(Context* ctx, char* op);
extern Token* consume_ident(Context* ctx);
extern void expect(Context* ctx, char* op);
extern int expect_number(Context* ctx);
extern bool at_eof(Context* ctx);

#endif