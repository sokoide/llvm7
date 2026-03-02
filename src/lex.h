#ifndef __LEX_H__
#define __LEX_H__

#include "common.h"

extern Token* new_token(TokenKind kind, Token* cur, const char* str, int len);
extern Token* tokenize(const char* p);
extern void lex_get_line_col(const char* source, const char* pos, int* line,
                             int* col);
extern void free_tokens(Token* head);
extern bool consume(Context* ctx, char* op);
extern Token* consume_ident(Context* ctx);
extern void expect(Context* ctx, char* op);
extern int expect_number(Context* ctx);
extern bool at_eof(Context* ctx);

#endif
