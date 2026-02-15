#include "variable.h"
#include <memory.h>

LVar* find_lvar(Context* ctx, Token* tok) {
    for (LVar* var = ctx->locals; var; var = var->next) {
        if (var->len == tok->len &&
            memcmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }
    return NULL;
}