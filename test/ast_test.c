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

    free_ast(node);
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

    free_ast(node);
    return NULL;
}

char* test_unary_num() {
    token = NULL;
    Token* head = tokenize("42");
    token = head;

    Node* node = unary();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_unary_plus() {
    token = NULL;
    Token* head = tokenize("+42");
    token = head;

    Node* node = unary();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_unary_minus() {
    token = NULL;
    Token* head = tokenize("-5");
    token = head;

    Node* node = unary();

    mu_assert("Node kind should be ND_SUB", node->kind == ND_SUB);
    mu_assert("Left value should be 0", node->lhs->val == 0);
    mu_assert("Right value should be 5", node->rhs->val == 5);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_unary_plus_num() {
    token = NULL;
    Token* head = tokenize("+5+3");
    token = head;

    Node* node = expr();

    mu_assert("Node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Left value should be 5", node->lhs->val == 5);
    mu_assert("Right value should be 3", node->rhs->val == 3);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_unary_minus_mul() {
    token = NULL;
    Token* head = tokenize("-3*4");
    token = head;

    Node* node = mul();

    mu_assert("Node kind should be ND_MUL", node->kind == ND_MUL);
    mu_assert("Right value should be 4", node->rhs->val == 4);
    mu_assert("Left node kind should be ND_SUB", node->lhs->kind == ND_SUB);
    mu_assert("Left-left value should be 0", node->lhs->lhs->val == 0);
    mu_assert("Left-right value should be 3", node->lhs->rhs->val == 3);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_primary_num() {
    token = NULL;
    Token* head = tokenize("42");
    token = head;

    Node* node = primary();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Token should be EOF", token->kind == TK_EOF);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_primary_paren() {
    token = NULL;
    Token* head = tokenize("(42)");
    token = head;

    Node* node = primary();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Token should be EOF", token->kind == TK_EOF);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_mul_single_num() {
    token = NULL;
    Token* head = tokenize("5");
    token = head;

    Node* node = mul();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 5", node->val == 5);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_mul_multiply() {
    token = NULL;
    Token* head = tokenize("2*3");
    token = head;

    Node* node = mul();

    mu_assert("Node kind should be ND_MUL", node->kind == ND_MUL);
    mu_assert("Left value should be 2", node->lhs->val == 2);
    mu_assert("Right value should be 3", node->rhs->val == 3);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_mul_divide() {
    token = NULL;
    Token* head = tokenize("6/2");
    token = head;

    Node* node = mul();

    mu_assert("Node kind should be ND_DIV", node->kind == ND_DIV);
    mu_assert("Left value should be 6", node->lhs->val == 6);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_mul_chain() {
    token = NULL;
    Token* head = tokenize("2*3*4");
    token = head;

    Node* node = mul();

    mu_assert("Root node kind should be ND_MUL", node->kind == ND_MUL);
    mu_assert("Right value should be 4", node->rhs->val == 4);
    mu_assert("Left node kind should be ND_MUL", node->lhs->kind == ND_MUL);
    mu_assert("Left-left value should be 2", node->lhs->lhs->val == 2);
    mu_assert("Left-right value should be 3", node->lhs->rhs->val == 3);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_expr_single_num() {
    token = NULL;
    Token* head = tokenize("42");
    token = head;

    Node* node = expr();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_expr_add() {
    token = NULL;
    Token* head = tokenize("1+2");
    token = head;

    Node* node = expr();

    mu_assert("Node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_expr_subtract() {
    token = NULL;
    Token* head = tokenize("5-3");
    token = head;

    Node* node = expr();

    mu_assert("Node kind should be ND_SUB", node->kind == ND_SUB);
    mu_assert("Left value should be 5", node->lhs->val == 5);
    mu_assert("Right value should be 3", node->rhs->val == 3);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_expr_add_sub_chain() {
    token = NULL;
    Token* head = tokenize("1-2+3");
    token = head;

    Node* node = expr();

    mu_assert("Root node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Right value should be 3", node->rhs->val == 3);
    mu_assert("Left node kind should be ND_SUB", node->lhs->kind == ND_SUB);
    mu_assert("Left-left value should be 1", node->lhs->lhs->val == 1);
    mu_assert("Left-right value should be 2", node->lhs->rhs->val == 2);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_expr_precedence() {
    token = NULL;
    Token* head = tokenize("1+2*3");
    token = head;

    Node* node = expr();

    mu_assert("Root node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right node kind should be ND_MUL", node->rhs->kind == ND_MUL);
    mu_assert("Right-left value should be 2", node->rhs->lhs->val == 2);
    mu_assert("Right-right value should be 3", node->rhs->rhs->val == 3);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_expr_complex_precedence() {
    token = NULL;
    Token* head = tokenize("1+2*3-4");
    token = head;

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

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_relational_lt() {
    token = NULL;
    Token* head = tokenize("1<2");
    token = head;

    Node* node = relational();

    mu_assert("Node kind should be ND_LT", node->kind == ND_LT);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_relational_le() {
    token = NULL;
    Token* head = tokenize("1<=2");
    token = head;

    Node* node = relational();

    mu_assert("Node kind should be ND_LE", node->kind == ND_LE);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_relational_gt() {
    token = NULL;
    Token* head = tokenize("1>2");
    token = head;

    Node* node = relational();

    mu_assert("Node kind should be ND_GT", node->kind == ND_GT);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_relational_ge() {
    token = NULL;
    Token* head = tokenize("1>=2");
    token = head;

    Node* node = relational();

    mu_assert("Node kind should be ND_GE", node->kind == ND_GE);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_equality_eq() {
    token = NULL;
    Token* head = tokenize("1==2");
    token = head;

    Node* node = equality();

    mu_assert("Node kind should be ND_EQ", node->kind == ND_EQ);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_equality_ne() {
    token = NULL;
    Token* head = tokenize("1!=2");
    token = head;

    Node* node = equality();

    mu_assert("Node kind should be ND_NE", node->kind == ND_NE);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_expr_combined_precedence() {
    token = NULL;
    Token* head = tokenize("1+2==3");
    token = head;

    Node* node = expr();

    mu_assert("Root node kind should be ND_EQ", node->kind == ND_EQ);
    mu_assert("Left node kind should be ND_ADD", node->lhs->kind == ND_ADD);
    mu_assert("Left-left value should be 1", node->lhs->lhs->val == 1);
    mu_assert("Left-right value should be 2", node->lhs->rhs->val == 2);
    mu_assert("Right value should be 3", node->rhs->val == 3);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_new_node_ident() {
    Node* node = new_node_ident("a");

    mu_assert("Node kind should be ND_LVAR", node->kind == ND_LVAR);
    mu_assert("Node offset should be 8", node->offset == 8);
    mu_assert("Node lhs should be NULL", node->lhs == NULL);
    mu_assert("Node rhs should be NULL", node->rhs == NULL);

    free_ast(node);
    return NULL;
}

char* test_new_node_ident_b() {
    Node* node = new_node_ident("b");

    mu_assert("Node kind should be ND_LVAR", node->kind == ND_LVAR);
    mu_assert("Node offset should be 16", node->offset == 16);

    free_ast(node);
    return NULL;
}

char* test_primary_ident() {
    token = NULL;
    Token* head = tokenize("a");
    token = head;

    Node* node = primary();

    mu_assert("Node kind should be ND_LVAR", node->kind == ND_LVAR);
    mu_assert("Node offset should be 8", node->offset == 8);
    mu_assert("Token should be EOF", token->kind == TK_EOF);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_assign() {
    token = NULL;
    Token* head = tokenize("a=42");
    token = head;

    Node* node = assign();

    mu_assert("Node kind should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Left offset should be 8", node->lhs->offset == 8);
    mu_assert("Right node kind should be ND_NUM", node->rhs->kind == ND_NUM);
    mu_assert("Right value should be 42", node->rhs->val == 42);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_assign_chain() {
    token = NULL;
    Token* head = tokenize("a=b=5");
    token = head;

    Node* node = assign();

    mu_assert("Root node kind should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Left offset should be 8", node->lhs->offset == 8);
    mu_assert("Right node kind should be ND_ASSIGN",
              node->rhs->kind == ND_ASSIGN);
    mu_assert("Right-left offset should be 16", node->rhs->lhs->offset == 16);
    mu_assert("Right-right value should be 5", node->rhs->rhs->val == 5);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_stmt() {
    token = NULL;
    Token* head = tokenize("42;");
    token = head;

    Node* node = stmt();

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Token should be EOF", token->kind == TK_EOF);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_stmt_assign() {
    token = NULL;
    Token* head = tokenize("a=5;");
    token = head;

    Node* node = stmt();

    mu_assert("Node kind should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Right node kind should be ND_NUM", node->rhs->kind == ND_NUM);

    token = NULL;
    free_ast(node);
    free_tokens(head);
    return NULL;
}

char* test_program_single_stmt() {
    token = NULL;
    Token* head = tokenize("42;");
    token = head;

    program();

    mu_assert("code[0] should not be NULL", code[0] != NULL);
    mu_assert("code[0] kind should be ND_NUM", code[0]->kind == ND_NUM);
    mu_assert("code[0] value should be 42", code[0]->val == 42);
    mu_assert("code[1] should be NULL", code[1] == NULL);

    token = NULL;
    free_ast(code[0]);
    free_tokens(head);
    return NULL;
}

char* test_program_multiple_stmts() {
    token = NULL;
    Token* head = tokenize("1;2;3;");
    token = head;

    program();

    mu_assert("code[0] should not be NULL", code[0] != NULL);
    mu_assert("code[0] kind should be ND_NUM", code[0]->kind == ND_NUM);
    mu_assert("code[0] value should be 1", code[0]->val == 1);
    mu_assert("code[1] should not be NULL", code[1] != NULL);
    mu_assert("code[1] kind should be ND_NUM", code[1]->kind == ND_NUM);
    mu_assert("code[1] value should be 2", code[1]->val == 2);
    mu_assert("code[2] should not be NULL", code[2] != NULL);
    mu_assert("code[2] kind should be ND_NUM", code[2]->kind == ND_NUM);
    mu_assert("code[2] value should be 3", code[2]->val == 3);
    mu_assert("code[3] should be NULL", code[3] == NULL);

    token = NULL;
    free_ast(code[0]);
    free_ast(code[1]);
    free_ast(code[2]);
    free_tokens(head);
    return NULL;
}

char* test_program_assign_stmts() {
    token = NULL;
    Token* head = tokenize("a=1;b=2;");
    token = head;

    program();

    mu_assert("code[0] should not be NULL", code[0] != NULL);
    mu_assert("code[0] kind should be ND_ASSIGN", code[0]->kind == ND_ASSIGN);
    mu_assert("code[1] should not be NULL", code[1] != NULL);
    mu_assert("code[1] kind should be ND_ASSIGN", code[1]->kind == ND_ASSIGN);
    mu_assert("code[2] should be NULL", code[2] == NULL);

    token = NULL;
    free_ast(code[0]);
    free_ast(code[1]);
    free_tokens(head);
    return NULL;
}
