#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "common.h"

#include <stdio.h>

extern LVar* find_lvar(Context* ctx, Token* tok);
extern LVar* find_gvar(Context* ctx, Token* tok);
extern LVar* add_lvar(Context* ctx, Token* tok, Type* type);
extern void reset_scope(void);
extern void enter_scope(void);
extern void leave_scope(void);

#endif
