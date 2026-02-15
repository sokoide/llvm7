#include "parse.h"
#include "lex.h"
#include "variable.h"
#include <stdlib.h>

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node* new_node_num(int val) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node* new_node_ident(Context* ctx, Token* tok) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    LVar* lvar = find_lvar(ctx, tok);
    if (lvar) {
        node->val = lvar->offset;
    } else {
        lvar = add_lvar(ctx, tok);
        node->val = lvar->offset;
    }

    return node;
}

void free_ast(Node* ast) {
    if (ast == NULL)
        return;
    free_ast(ast->lhs);
    free_ast(ast->rhs);
    free(ast);
}

void program(Context* ctx) {
    int i = 0;
    while (!at_eof(ctx)) {
        ctx->code[i++] = stmt(ctx);
    }
    ctx->node_count = i;
}

Node* stmt(Context* ctx) {
    Node* node;
    if (consume(ctx, "return")) {
        node = new_node(ND_RETURN, expr(ctx), NULL);
    } else if (consume(ctx, "if")) {
        expect(ctx, "(");
        Node* cond = expr(ctx);
        expect(ctx, ")");
        Node* then = stmt(ctx);
        Node* els = NULL;
        if (consume(ctx, "else")) {
            els = stmt(ctx);
        }
        node = new_node(ND_IF, then, els);
        node->cond = cond;
        return node;
    } else if (consume(ctx, "{")) {
        // Block statement: consume statements until }
        node = stmt(ctx);
        expect(ctx, "}");
        return node;
    } else {
        node = expr(ctx);
    }
    expect(ctx, ";");

    return node;
}

Node* expr(Context* ctx) { return assign(ctx); }

Node* assign(Context* ctx) {
    Node* node = equality(ctx);
    if (consume(ctx, "=")) {
        node = new_node(ND_ASSIGN, node, assign(ctx));
    }
    return node;
}

Node* equality(Context* ctx) {
    Node* node = relational(ctx);
    while (1) {
        if (consume(ctx, "==")) {
            node = new_node(ND_EQ, node, relational(ctx));
        } else if (consume(ctx, "!=")) {
            node = new_node(ND_NE, node, relational(ctx));
        } else {
            return node;
        }
    }
}

Node* relational(Context* ctx) {
    Node* node = add(ctx);
    while (1) {
        if (consume(ctx, "<=")) {
            node = new_node(ND_LE, node, add(ctx));
        } else if (consume(ctx, ">=")) {
            node = new_node(ND_GE, node, add(ctx));
        } else if (consume(ctx, "<")) {
            node = new_node(ND_LT, node, add(ctx));
        } else if (consume(ctx, ">")) {
            node = new_node(ND_GT, node, add(ctx));
        } else {
            return node;
        }
    }
}

Node* add(Context* ctx) {
    Node* node = mul(ctx);
    while (1) {
        if (consume(ctx, "+")) {
            node = new_node(ND_ADD, node, mul(ctx));
        } else if (consume(ctx, "-")) {
            node = new_node(ND_SUB, node, mul(ctx));
        } else {
            return node;
        }
    }
}

Node* mul(Context* ctx) {
    Node* node = unary(ctx);
    while (1) {
        if (consume(ctx, "*")) {
            node = new_node(ND_MUL, node, unary(ctx));
        } else if (consume(ctx, "/")) {
            node = new_node(ND_DIV, node, unary(ctx));
        } else {
            return node;
        }
    }
}

Node* unary(Context* ctx) {
    if (consume(ctx, "+")) {
        return primary(ctx);
    } else if (consume(ctx, "-")) {
        return new_node(ND_SUB, new_node_num(0), primary(ctx));
    } else {
        return primary(ctx);
    }
}

Node* primary(Context* ctx) {
    // "(" expr ")"
    if (consume(ctx, "(")) {
        Node* node = expr(ctx);
        expect(ctx, ")");
        return node;
    }

    // ident
    Token* tok = consume_ident(ctx);
    if (tok) {
        return new_node_ident(ctx, tok);
    }

    // number
    return new_node_num(expect_number(ctx));
}
