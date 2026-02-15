#include "ast.h"
#include "lexer.h"
#include <stdlib.h>

Node* code[MAX_NODES];

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

Node* new_node_ident(const char* name) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->offset = (name[0] - 'a' + 1) * 8;
    return node;
}

void free_ast(Node* ast) {
    if (ast == NULL)
        return;
    free_ast(ast->lhs);
    free_ast(ast->rhs);
    free(ast);
}

void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

Node* stmt() {
    Node* node = expr();
    expect(";");
    return node;
}

Node* expr() { return assign(); }

Node* assign() {
    Node* node = equality();
    if (consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

Node* equality() {
    Node* node = relational();
    while (1) {
        if (consume("==")) {
            node = new_node(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_node(ND_NE, node, relational());
        } else {
            return node;
        }
    }
}

Node* relational() {
    Node* node = add();
    while (1) {
        if (consume("<=")) {
            node = new_node(ND_LE, node, add());
        } else if (consume(">=")) {
            node = new_node(ND_GE, node, add());
        } else if (consume("<")) {
            node = new_node(ND_LT, node, add());
        } else if (consume(">")) {
            node = new_node(ND_GT, node, add());
        } else {
            return node;
        }
    }
}

Node* add() {
    Node* node = mul();
    while (1) {
        if (consume("+")) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

Node* mul() {
    Node* node = unary();
    while (1) {
        if (consume("*")) {
            node = new_node(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_node(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

Node* unary() {
    if (consume("+")) {
        return primary();
    } else if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    } else {
        return primary();
    }
}

Node* primary() {
    // "(" expr ")"
    if (consume("(")) {
        Node* node = expr();
        expect(")");
        return node;
    }

    // ident
    Token* tok = consume_ident();
    if (tok) {
        return new_node_ident(tok->str);
    }

    // number
    return new_node_num(expect_number());
}
