#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "common.h"

#include <stdio.h>

extern LVar* find_lvar(Context* ctx, Token* tok);
extern LVar* add_lvar(Context* ctx, Token* tok, Type* type);

#endif