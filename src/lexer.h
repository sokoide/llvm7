#ifndef __LEXER_H__
#define __LEXER_H__

#include "common.h"

extern Token* tokenize(const char* p);
extern void free_tokens(Token* head);
extern bool consume(Context* ctx, char* op);
extern Token* consume_ident(Context* ctx);
extern void expect(Context* ctx, char* op);
extern int expect_number(Context* ctx);
extern bool at_eof(Context* ctx);

#endif
