#include "parse.h"
#include "lex.h"
#include "variable.h"
#include <stdlib.h>

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->next = NULL;
    node->lhs = lhs;
    node->rhs = rhs;
    node->init = NULL;
    node->cond = NULL;
    node->val = 0;

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
    // Don't free ast->lhs for ND_CALL (it's a Token*, not a Node*)
    if (ast->kind != ND_CALL) {
        free_ast(ast->lhs);
    }
    free_ast(ast->rhs);
    free_ast(ast->next);
    free_ast(ast->cond);
    free_ast(ast->init);
    free(ast);
}

// params = ident ("," ident)*
Node* params(Context* ctx) {
    Token* tok = consume_ident(ctx);
    if (!tok) {
        return NULL;
    }

    Node* head = new_node_ident(ctx, tok);
    Node* tail = head;

    while (consume(ctx, ",")) {
        tok = consume_ident(ctx);
        if (!tok) {
            fprintf(stderr, "Expected parameter name\n");
            exit(1);
        }
        tail->next = new_node_ident(ctx, tok);
        tail = tail->next;
    }

    return head;
}

// function = ident "(" params? ")" "{" stmt* "}"
Node* function(Context* ctx) {
    Token* tok = consume_ident(ctx);
    if (!tok) {
        fprintf(stderr, "Expected function name\n");
        exit(1);
    }

    expect(ctx, "(");

    // Parse parameters
    Node* func_params = NULL;
    if (!consume(ctx, ")")) {
        func_params = params(ctx);
        expect(ctx, ")");
    }

    expect(ctx, "{");

    // Create function node
    Node* node = new_node(ND_FUNCTION, NULL, NULL);
    node->tok = tok;
    node->rhs = func_params; // Store parameters in rhs

    // Parse function body (statements)
    Node* head = NULL;
    Node* tail = NULL;
    while (!consume(ctx, "}")) {
        Node* stmt_node = stmt(ctx);
        if (head == NULL) {
            head = stmt_node;
            tail = stmt_node;
        } else {
            tail->next = stmt_node;
            tail = stmt_node;
        }
    }
    node->lhs = head;

    return node;
}

void program(Context* ctx) {
    int i = 0;
    while (!at_eof(ctx)) {
        ctx->code[i++] = function(ctx);
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
    } else if (consume(ctx, "while")) {
        expect(ctx, "(");
        Node* cond = expr(ctx);
        expect(ctx, ")");
        Node* body = stmt(ctx);
        node = new_node(ND_WHILE, body, NULL);
        node->cond = cond;
        return node;
    } else if (consume(ctx, "for")) {
        expect(ctx, "(");
        Node* init = NULL;
        if (!consume(ctx, ";")) {
            init = expr(ctx);
            expect(ctx, ";");
        }
        Node* cond = NULL;
        if (!consume(ctx, ";")) {
            cond = expr(ctx);
            expect(ctx, ";");
        }
        Node* inc = NULL;
        if (!consume(ctx, ")")) {
            inc = expr(ctx);
            expect(ctx, ")");
        }
        Node* body = stmt(ctx);
        node = new_node(ND_FOR, body, inc);
        node->init = init;
        node->cond = cond;
        return node;
    } else if (consume(ctx, "{")) {
        // Block statement: consume statements until }
        node = stmt(ctx);
        Node* head = node;
        while (!consume(ctx, "}")) {
            node->next = stmt(ctx);
            node = node->next;
        }
        return new_node(ND_BLOCK, head, NULL);
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
    } else if (consume(ctx, "*")) {
        return new_node(ND_DEREF, unary(ctx), NULL);
    } else if (consume(ctx, "&")) {
        return new_node(ND_ADDR, unary(ctx), NULL);
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

    // function call or ident
    Token* tok = consume_ident(ctx);

    if (tok) {
        if (consume(ctx, "(")) {
            Node* node_args = NULL;
            if (!consume(ctx, ")")) {
                // args
                node_args = args(ctx);
                expect(ctx, ")");
            }
            // function call
            Node* node_func = new_node(ND_CALL, node_args, NULL);
            node_func->tok = tok;
            return node_func;
        }
        // ident
        return new_node_ident(ctx, tok);
    }

    // number
    return new_node_num(expect_number(ctx));
}

Node* args(Context* ctx) {
    Node* node = expr(ctx);
    Node* head = node;
    while (consume(ctx, ",")) {
        node->next = expr(ctx);
        node = node->next;
    }
    return head;
}