#include "variable.h"
#include <memory.h>
#include <stdlib.h>

typedef struct ScopedLVar ScopedLVar;
struct ScopedLVar {
    LVar base;
    int scope_depth;
    LVar* scope_next;
};

LVar* find_lvar(Context* ctx, Token* tok) {
    for (LVar* var = ctx->active_locals; var != NULL;) {
        ScopedLVar* scoped = (ScopedLVar*)var;
        if (var->len == tok->len &&
            memcmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
        var = scoped->scope_next;
    }
    return NULL;
}

LVar* find_gvar(Context* ctx, Token* tok) {
    for (LVar* var = ctx->globals; var != NULL; var = var->next) {
        if (var->len == tok->len &&
            memcmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }
    return NULL;
}

LVar* add_lvar(Context* ctx, Token* tok, Type* type) {
    ScopedLVar* scoped = calloc(1, sizeof(ScopedLVar));
    if (!scoped) {
        perror("calloc");
        exit(1);
    }
    LVar* new_var = &scoped->base;
    new_var->name = tok->str;
    new_var->len = tok->len;
    new_var->type = type;
    scoped->scope_depth = ctx->scope_depth;

    // Assign a unique slot id for codegen local_allocas[] indexing.
    if (ctx->next_local_offset >= MAX_LOCALS) {
        fprintf(stderr, "Too many local variables (max %d)\n", MAX_LOCALS);
        exit(1);
    }
    new_var->offset = ctx->next_local_offset++;

    // Keep all declarations for codegen and a separate stack for lookup.
    scoped->scope_next = ctx->active_locals;
    ctx->active_locals = new_var;
    new_var->next = ctx->locals;
    ctx->locals = new_var;

    return new_var;
}

void reset_scope(Context* ctx) {
    ctx->active_locals = NULL;
    ctx->scope_depth = 0;
    ctx->next_local_offset = 0;
}

void enter_scope(Context* ctx) { ctx->scope_depth++; }

void leave_scope(Context* ctx) {
    if (ctx->scope_depth > 0) {
        // Remove each declaration from lookup once, without losing its slot.
        while (ctx->active_locals) {
            ScopedLVar* scoped = (ScopedLVar*)ctx->active_locals;
            if (scoped->scope_depth < ctx->scope_depth) {
                break;
            }
            ctx->active_locals = scoped->scope_next;
        }
        ctx->scope_depth--;
    }
}
