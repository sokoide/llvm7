#include "parse.h"
#include "lex.h"
#include "variable.h"
#include <stdlib.h>
#include <string.h>

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
    node->type = new_type_int();
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
    node->type = lvar->type; // Store type

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

// parse_type = "int" | "void" | type "*"
Type* parse_type(Context* ctx);

// Helper to create a new int type
Type* new_type_int(void) {
    Type* t = calloc(1, sizeof(Type));
    t->ty = INT;
    t->ptr_to = NULL;
    return t;
}

// Helper to create a pointer type
Type* new_type_ptr(Type* base) {
    Type* t = calloc(1, sizeof(Type));
    t->ty = PTR;
    t->ptr_to = base;
    return t;
}

// Try to parse a type, returns NULL if not a type
Type* try_parse_type(Context* ctx) {
    // Parse base type
    Type* base = NULL;
    if (consume(ctx, "int")) {
        base = new_type_int();
    } else if (consume(ctx, "void")) {
        base = new_type_int();
        base->ty = 1; // Use 1 for void (not INT)
    } else {
        return NULL; // Not a type
    }

    // Parse pointers
    while (consume(ctx, "*")) {
        base = new_type_ptr(base);
    }

    return base;
}

Type* parse_type(Context* ctx) {
    Type* base = try_parse_type(ctx);
    if (!base) {
        fprintf(stderr, "Expected type (int or void)\n");
        exit(1);
    }
    return base;
}

// params = ty ident ("," ty ident)*
Node* parse_params(Context* ctx) {
    Type* ty = parse_type(ctx);
    Token* tok = consume_ident(ctx);
    if (!tok) {
        fprintf(stderr, "Expected parameter name\n");
        exit(1);
    }

    // Add parameter to locals before creating node
    add_lvar(ctx, tok, ty);
    Node* head = new_node_ident(ctx, tok);
    head->type = ty; // Store type in node
    Node* tail = head;

    while (consume(ctx, ",")) {
        ty = parse_type(ctx);
        tok = consume_ident(ctx);
        if (!tok) {
            fprintf(stderr, "Expected parameter name\n");
            exit(1);
        }
        // Add parameter to locals before creating node
        add_lvar(ctx, tok, ty);
        tail->next = new_node_ident(ctx, tok);
        tail->next->type = ty; // Store type in node
        tail = tail->next;
    }

    return head;
}

// function = ty ident "(" params? ")" "{" stmt* "}"
Node* parse_function(Context* ctx) {
    Type* return_ty = parse_type(ctx);
    Token* tok = consume_ident(ctx);
    if (!tok) {
        fprintf(stderr, "Expected function name\n");
        exit(1);
    }

    // Reset locals for each function
    ctx->locals = NULL;

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
    node->type = return_ty;  // Store return type
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
    node->locals = ctx->locals; // Store local variables

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

    // Check for type declaration: int ident ";"
    if (consume(ctx, "int")) {
        // Check for pointers (e.g., int* x)
        Type* ty = new_type_int();
        while (consume(ctx, "*")) {
            ty = new_type_ptr(ty);
        }

        Token* tok = consume_ident(ctx);
        if (!tok) {
            fprintf(stderr, "Expected variable name after type\n");
            exit(1);
        }

        expect(ctx, ";");

        // Add variable to locals
        LVar* lvar = add_lvar(ctx, tok, ty);
        Node* decl = new_node(ND_DECL, NULL, NULL);
        decl->tok = tok;
        decl->val = lvar->offset;
        decl->type = ty;
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
        if (node->lhs) {
            node->type = node->lhs->type;
        }
    }
    return node;
}

Node* parse_equality(Context* ctx) {
    Node* node = parse_relational(ctx);
    while (1) {
        if (consume(ctx, "==")) {
            node = new_node(ND_EQ, node, parse_relational(ctx));
            node->type = new_type_int();
        } else if (consume(ctx, "!=")) {
            node = new_node(ND_NE, node, parse_relational(ctx));
            node->type = new_type_int();
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
            node->type = new_type_int();
        } else if (consume(ctx, ">=")) {
            node = new_node(ND_GE, node, parse_add(ctx));
            node->type = new_type_int();
        } else if (consume(ctx, "<")) {
            node = new_node(ND_LT, node, parse_add(ctx));
            node->type = new_type_int();
        } else if (consume(ctx, ">")) {
            node = new_node(ND_GT, node, parse_add(ctx));
            node->type = new_type_int();
        } else {
            return node;
        }
    }
}

Node* parse_add(Context* ctx) {
    Node* node = parse_mul(ctx);
    while (1) {
        if (consume(ctx, "+")) {
            Node* rhs = parse_mul(ctx);
            Node* newNode = new_node(ND_ADD, node, rhs);
            if (node->type && node->type->ty == PTR) {
                newNode->type = node->type;
            } else if (rhs->type && rhs->type->ty == PTR) {
                newNode->type = rhs->type;
            } else {
                newNode->type = new_type_int();
            }
            node = newNode;
        } else if (consume(ctx, "-")) {
            Node* rhs = parse_mul(ctx);
            Node* newNode = new_node(ND_SUB, node, rhs);
            if (node->type && node->type->ty == PTR) {
                newNode->type = node->type;
            } else {
                newNode->type = new_type_int();
            }
            node = newNode;
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
            node->type = new_type_int();
        } else if (consume(ctx, "/")) {
            node = new_node(ND_DIV, node, parse_unary(ctx));
            node->type = new_type_int();
        } else {
            return node;
        }
    }
}

static int type_size(Type* ty) {
    if (ty == NULL)
        return 4;
    if (ty->ty == PTR)
        return 8;
    return 4;
}

Node* parse_unary(Context* ctx) {
    if (consume(ctx, "+")) {
        return parse_primary(ctx);
    } else if (consume(ctx, "-")) {
        Node* node = new_node(ND_SUB, new_node_num(0), parse_primary(ctx));
        node->type = new_type_int();
        return node;
    } else if (consume(ctx, "sizeof")) {
        if (consume(ctx, "(")) {
            // Check if it's a type name
            Type* ty = try_parse_type(ctx);
            if (ty) {
                expect(ctx, ")");
                return new_node_num(type_size(ty));
            }
            // If not a type name, backtrack?
            // Wait, currently we don't have backtrack.
            // But if try_parse_type fails it will return NULL.
            // If it returns NULL, then it must be an expression.
            Node* node = parse_expr(ctx);
            expect(ctx, ")");
            return new_node_num(type_size(node->type));
        } else {
            Node* node = parse_unary(ctx);
            return new_node_num(type_size(node->type));
        }
    } else if (consume(ctx, "*")) {
        Node* operand = parse_unary(ctx);
        Node* node = new_node(ND_DEREF, operand, NULL);
        if (operand->type && operand->type->ty == PTR) {
            node->type = operand->type->ptr_to;
        } else {
            node->type = new_type_int();
        }
        return node;
    } else if (consume(ctx, "&")) {
        Node* operand = parse_unary(ctx);
        Node* node = new_node(ND_ADDR, operand, NULL);
        node->type =
            new_type_ptr(operand->type ? operand->type : new_type_int());
        return node;
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
            node_func->type = new_type_int();
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