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
    if (!lvar) {
        // Undeclared variable - error
        fprintf(stderr, "Error: undeclared variable '");
        fwrite(tok->str, 1, tok->len, stderr);
        fprintf(stderr, "'\n");
        exit(1);
    }
    node->val = lvar->offset;

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

// parse_type = "int" | "void"
Type parse_type(Context* ctx) {
    if (consume(ctx, "int")) {
        return TY_INT;
    } else if (consume(ctx, "void")) {
        return TY_VOID;
    }
    fprintf(stderr, "Expected type (int or void)\n");
    exit(1);
}

// params = ty ident ("," ty ident)*
Node* parse_params(Context* ctx) {
    (void)parse_type(ctx); // Parse type (currently all params are int)
    Token* tok = consume_ident(ctx);
    if (!tok) {
        fprintf(stderr, "Expected parameter name\n");
        exit(1);
    }

    // Add parameter to locals before creating node
    add_lvar(ctx, tok);
    Node* head = new_node_ident(ctx, tok);
    Node* tail = head;

    while (consume(ctx, ",")) {
        (void)parse_type(ctx); // Parse type (currently all params are int)
        tok = consume_ident(ctx);
        if (!tok) {
            fprintf(stderr, "Expected parameter name\n");
            exit(1);
        }
        // Add parameter to locals before creating node
        add_lvar(ctx, tok);
        tail->next = new_node_ident(ctx, tok);
        tail = tail->next;
    }

    return head;
}

// function = ty ident "(" params? ")" "{" stmt* "}"
Node* parse_function(Context* ctx) {
    Type return_ty = parse_type(ctx);
    Token* tok = consume_ident(ctx);
    if (!tok) {
        fprintf(stderr, "Expected function name\n");
        exit(1);
    }

    expect(ctx, "(");

    // Parse parameters
    Node* func_params = NULL;
    if (!consume(ctx, ")")) {
        func_params = parse_params(ctx);
        expect(ctx, ")");
    }

    expect(ctx, "{");

    // Create function node with return type
    Node* node = new_node(ND_FUNCTION, NULL, NULL);
    node->tok = tok;
    node->val = return_ty;   // Store return type in val field
    node->rhs = func_params; // Store parameters in rhs

    // Parse function body (statements)
    Node* head = NULL;
    Node* tail = NULL;
    while (!consume(ctx, "}")) {
        Node* stmt_node = parse_stmt(ctx);
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

void parse_program(Context* ctx) {
    int i = 0;
    while (!at_eof(ctx)) {
        ctx->code[i++] = parse_function(ctx);
    }
    ctx->node_count = i;
}

Node* parse_stmt(Context* ctx) {
    Node* node;

    // Check for type declaration: ty ident ";"
    if (consume(ctx, "int")) {
        Token* tok = consume_ident(ctx);
        if (!tok) {
            fprintf(stderr, "Expected variable name after type\n");
            exit(1);
        }
        expect(ctx, ";");
        // Add variable to locals
        LVar* lvar = add_lvar(ctx, tok);
        Node* decl = new_node(ND_DECL, NULL, NULL);
        decl->tok = tok;
        decl->val = lvar->offset; // Store offset
        return decl;
    }

    if (consume(ctx, "return")) {
        node = new_node(ND_RETURN, parse_expr(ctx), NULL);
    } else if (consume(ctx, "if")) {
        expect(ctx, "(");
        Node* cond = parse_expr(ctx);
        expect(ctx, ")");
        Node* then = parse_stmt(ctx);
        Node* els = NULL;
        if (consume(ctx, "else")) {
            els = parse_stmt(ctx);
        }
        node = new_node(ND_IF, then, els);
        node->cond = cond;
        return node;
    } else if (consume(ctx, "while")) {
        expect(ctx, "(");
        Node* cond = parse_expr(ctx);
        expect(ctx, ")");
        Node* body = parse_stmt(ctx);
        node = new_node(ND_WHILE, body, NULL);
        node->cond = cond;
        return node;
    } else if (consume(ctx, "for")) {
        expect(ctx, "(");
        Node* init = NULL;
        if (!consume(ctx, ";")) {
            init = parse_expr(ctx);
            expect(ctx, ";");
        }
        Node* cond = NULL;
        if (!consume(ctx, ";")) {
            cond = parse_expr(ctx);
            expect(ctx, ";");
        }
        Node* inc = NULL;
        if (!consume(ctx, ")")) {
            inc = parse_expr(ctx);
            expect(ctx, ")");
        }
        Node* body = parse_stmt(ctx);
        node = new_node(ND_FOR, body, inc);
        node->init = init;
        node->cond = cond;
        return node;
    } else if (consume(ctx, "{")) {
        // Block statement: consume statements until }
        node = parse_stmt(ctx);
        Node* head = node;
        while (!consume(ctx, "}")) {
            node->next = parse_stmt(ctx);
            node = node->next;
        }
        return new_node(ND_BLOCK, head, NULL);
    } else {
        node = parse_expr(ctx);
    }
    expect(ctx, ";");

    return node;
}

Node* parse_expr(Context* ctx) { return parse_assign(ctx); }

Node* parse_assign(Context* ctx) {
    Node* node = parse_equality(ctx);
    if (consume(ctx, "=")) {
        node = new_node(ND_ASSIGN, node, parse_assign(ctx));
    }
    return node;
}

Node* parse_equality(Context* ctx) {
    Node* node = parse_relational(ctx);
    while (1) {
        if (consume(ctx, "==")) {
            node = new_node(ND_EQ, node, parse_relational(ctx));
        } else if (consume(ctx, "!=")) {
            node = new_node(ND_NE, node, parse_relational(ctx));
        } else {
            return node;
        }
    }
}

Node* parse_relational(Context* ctx) {
    Node* node = parse_add(ctx);
    while (1) {
        if (consume(ctx, "<=")) {
            node = new_node(ND_LE, node, parse_add(ctx));
        } else if (consume(ctx, ">=")) {
            node = new_node(ND_GE, node, parse_add(ctx));
        } else if (consume(ctx, "<")) {
            node = new_node(ND_LT, node, parse_add(ctx));
        } else if (consume(ctx, ">")) {
            node = new_node(ND_GT, node, parse_add(ctx));
        } else {
            return node;
        }
    }
}

Node* parse_add(Context* ctx) {
    Node* node = parse_mul(ctx);
    while (1) {
        if (consume(ctx, "+")) {
            node = new_node(ND_ADD, node, parse_mul(ctx));
        } else if (consume(ctx, "-")) {
            node = new_node(ND_SUB, node, parse_mul(ctx));
        } else {
            return node;
        }
    }
}

Node* parse_mul(Context* ctx) {
    Node* node = parse_unary(ctx);
    while (1) {
        if (consume(ctx, "*")) {
            node = new_node(ND_MUL, node, parse_unary(ctx));
        } else if (consume(ctx, "/")) {
            node = new_node(ND_DIV, node, parse_unary(ctx));
        } else {
            return node;
        }
    }
}

Node* parse_unary(Context* ctx) {
    if (consume(ctx, "+")) {
        return parse_primary(ctx);
    } else if (consume(ctx, "-")) {
        return new_node(ND_SUB, new_node_num(0), parse_primary(ctx));
    } else if (consume(ctx, "*")) {
        return new_node(ND_DEREF, parse_unary(ctx), NULL);
    } else if (consume(ctx, "&")) {
        return new_node(ND_ADDR, parse_unary(ctx), NULL);
    } else {
        return parse_primary(ctx);
    }
}

Node* parse_primary(Context* ctx) {
    // "(" expr ")"
    if (consume(ctx, "(")) {
        Node* node = parse_expr(ctx);
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
                node_args = parse_args(ctx);
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

Node* parse_args(Context* ctx) {
    Node* node = parse_expr(ctx);
    Node* head = node;
    while (consume(ctx, ",")) {
        node->next = parse_expr(ctx);
        node = node->next;
    }
    return head;
}