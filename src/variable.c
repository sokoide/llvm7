#include "variable.h"
#include <memory.h>
#include <stdlib.h>

typedef struct ScopedLVar ScopedLVar;
struct ScopedLVar {
    LVar base;
    int scope_depth;
};

static int current_scope_depth = 0;

LVar *find_lvar(Context *ctx, Token *tok) {
    for (LVar *var = ctx->locals; var != NULL; var = var->next) {
        ScopedLVar *scoped = (ScopedLVar *)var;
        if (scoped->scope_depth <= current_scope_depth &&
            var->len == tok->len &&
            memcmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }
    return NULL;
}

LVar *find_gvar(Context *ctx, Token *tok) {
    for (LVar *var = ctx->globals; var != NULL; var = var->next) {
        if (var->len == tok->len &&
            memcmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }
    return NULL;
}

LVar *add_lvar(Context *ctx, Token *tok, Type *type) {
    ScopedLVar *scoped = calloc(1, sizeof(ScopedLVar));
    LVar *new_var = &scoped->base;
    new_var->name = tok->str;
    new_var->len = tok->len;
    new_var->type = type;
    scoped->scope_depth = current_scope_depth;

    // Assign unique slot id for codegen local_allocas[] indexing.
    int next_offset = 0;
    for (LVar *var = ctx->locals; var != NULL; var = var->next) {
        if (var->offset >= next_offset) {
            next_offset = var->offset + 1;
        }
    }
    new_var->offset = next_offset;

    // Push front so name lookup finds the most recent declaration first.
    new_var->next = ctx->locals;
    ctx->locals = new_var;

    return new_var;
}

void reset_scope(void) { current_scope_depth = 0; }

void enter_scope(void) { current_scope_depth++; }

void leave_scope(void) {
    if (current_scope_depth > 0) {
        current_scope_depth--;
    }
}
