#include "variable.h"
#include <memory.h>
#include <stdlib.h>

LVar* find_lvar(Context* ctx, Token* tok) {
    for (LVar* var = ctx->locals; var != NULL; var = var->next) {
        if (var->len == tok->len &&
            memcmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }
    return NULL;
}

LVar* add_lvar(Context* ctx, Token* tok, Type* type) {
    LVar* new_var = calloc(1, sizeof(LVar));
    new_var->name = tok->str;
    new_var->len = tok->len;
    new_var->next = NULL;
    new_var->type = type;

    if (ctx->locals == NULL) {
        new_var->offset = 0;
        ctx->locals = new_var;
        return new_var;
    }

    LVar* var = ctx->locals;
    int offset = 0;
    while (var->next != NULL) {
        offset += 1;
        var = var->next;
    }
    new_var->offset = offset + 1;
    var->next = new_var;

    return new_var;
}
