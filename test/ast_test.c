#include "ast_test.h"
#include "../src/lexer.h"
#include "test_common.h"
#include <stdio.h>

char* test_new_node_num() {
    Node* node = new_node_num(42);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Node lhs should be NULL", node->lhs == NULL);
    mu_assert("Node rhs should be NULL", node->rhs == NULL);

    free(node);
    return NULL;
}

char* test_new_node() {
    Node* lhs = new_node_num(5);
    Node* rhs = new_node_num(3);
    Node* node = new_node(ND_ADD, lhs, rhs);

    mu_assert("Node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Node lhs should be lhs node", node->lhs == lhs);
    mu_assert("Node rhs should be rhs node", node->rhs == rhs);
    mu_assert("Node lhs value should be 5", node->lhs->val == 5);
    mu_assert("Node rhs value should be 3", node->rhs->val == 3);

    free(node);
    free(lhs);
    free(rhs);
    return NULL;
}

char* test_primary_num() {
    token = NULL;
    token = tokenize("42");

    Node* node = primary();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Token should be EOF", token->kind == TK_EOF);

    free(node);
    return NULL;
}

char* test_primary_paren() {
    token = NULL;
    token = tokenize("(42)");

    Node* node = primary();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Token should be EOF", token->kind == TK_EOF);

    free(node);
    return NULL;
}

char* test_mul_single_num() {
    token = NULL;
    token = tokenize("5");

    Node* node = mul();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 5", node->val == 5);

    free(node);
    return NULL;
}

char* test_mul_multiply() {
    token = NULL;
    token = tokenize("2*3");

    Node* node = mul();

    mu_assert("Node kind should be ND_MUL", node->kind == ND_MUL);
    mu_assert("Left value should be 2", node->lhs->val == 2);
    mu_assert("Right value should be 3", node->rhs->val == 3);

    free(node->rhs);
    free(node->lhs);
    free(node);
    return NULL;
}

char* test_mul_divide() {
    token = NULL;
    token = tokenize("6/2");

    Node* node = mul();

    mu_assert("Node kind should be ND_DIV", node->kind == ND_DIV);
    mu_assert("Left value should be 6", node->lhs->val == 6);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    free(node->rhs);
    free(node->lhs);
    free(node);
    return NULL;
}

char* test_mul_chain() {
    token = NULL;
    token = tokenize("2*3*4");

    Node* node = mul();

    mu_assert("Root node kind should be ND_MUL", node->kind == ND_MUL);
    mu_assert("Right value should be 4", node->rhs->val == 4);
    mu_assert("Left node kind should be ND_MUL", node->lhs->kind == ND_MUL);
    mu_assert("Left-left value should be 2", node->lhs->lhs->val == 2);
    mu_assert("Left-right value should be 3", node->lhs->rhs->val == 3);

    free(node->rhs);
    free(node->lhs->rhs);
    free(node->lhs->lhs);
    free(node->lhs);
    free(node);
    return NULL;
}

char* test_expr_single_num() {
    token = NULL;
    token = tokenize("42");

    Node* node = expr();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);

    free(node);
    return NULL;
}

char* test_expr_add() {
    token = NULL;
    token = tokenize("1+2");

    Node* node = expr();

    mu_assert("Node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    free(node->rhs);
    free(node->lhs);
    free(node);
    return NULL;
}

char* test_expr_subtract() {
    token = NULL;
    token = tokenize("5-3");

    Node* node = expr();

    mu_assert("Node kind should be ND_SUB", node->kind == ND_SUB);
    mu_assert("Left value should be 5", node->lhs->val == 5);
    mu_assert("Right value should be 3", node->rhs->val == 3);

    free(node->rhs);
    free(node->lhs);
    free(node);
    return NULL;
}

char* test_expr_add_sub_chain() {
    token = NULL;
    token = tokenize("1-2+3");

    Node* node = expr();

    mu_assert("Root node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Right value should be 3", node->rhs->val == 3);
    mu_assert("Left node kind should be ND_SUB", node->lhs->kind == ND_SUB);
    mu_assert("Left-left value should be 1", node->lhs->lhs->val == 1);
    mu_assert("Left-right value should be 2", node->lhs->rhs->val == 2);

    free(node->rhs);
    free(node->lhs->rhs);
    free(node->lhs->lhs);
    free(node->lhs);
    free(node);
    return NULL;
}

char* test_expr_precedence() {
    token = NULL;
    token = tokenize("1+2*3");

    Node* node = expr();

    mu_assert("Root node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right node kind should be ND_MUL", node->rhs->kind == ND_MUL);
    mu_assert("Right-left value should be 2", node->rhs->lhs->val == 2);
    mu_assert("Right-right value should be 3", node->rhs->rhs->val == 3);

    free(node->rhs->rhs);
    free(node->rhs->lhs);
    free(node->rhs);
    free(node->lhs);
    free(node);
    return NULL;
}

char* test_expr_complex_precedence() {
    token = NULL;
    token = tokenize("1+2*3-4");

    Node* node = expr();

    mu_assert("Root node kind should be ND_SUB", node->kind == ND_SUB);
    mu_assert("Right value should be 4", node->rhs->val == 4);
    mu_assert("Left node kind should be ND_ADD", node->lhs->kind == ND_ADD);
    mu_assert("Left-left value should be 1", node->lhs->lhs->val == 1);
    mu_assert("Left-right node kind should be ND_MUL",
              node->lhs->rhs->kind == ND_MUL);
    mu_assert("Left-right-left value should be 2",
              node->lhs->rhs->lhs->val == 2);
    mu_assert("Left-right-right value should be 3",
              node->lhs->rhs->rhs->val == 3);

    free(node->rhs);
    free(node->lhs->rhs->rhs);
    free(node->lhs->rhs->lhs);
    free(node->lhs->rhs);
    free(node->lhs->lhs);
    free(node->lhs);
    free(node);
    return NULL;
}
