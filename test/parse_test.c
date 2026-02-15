#include "parse_test.h"
#include "../src/lex.h"
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
    Context ctx = {0};
    ctx.current_token = tokenize("42");

    Node* node = unary(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_unary_plus() {
    Context ctx = {0};
    ctx.current_token = tokenize("+42");

    Node* node = unary(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_unary_minus() {
    Context ctx = {0};
    ctx.current_token = tokenize("-5");

    Node* node = unary(&ctx);

    mu_assert("Node kind should be ND_SUB", node->kind == ND_SUB);
    mu_assert("Left value should be 0", node->lhs->val == 0);
    mu_assert("Right value should be 5", node->rhs->val == 5);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_unary_plus_num() {
    Context ctx = {0};
    ctx.current_token = tokenize("+5+3");

    Node* node = expr(&ctx);

    mu_assert("Node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Left value should be 5", node->lhs->val == 5);
    mu_assert("Right value should be 3", node->rhs->val == 3);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_unary_minus_mul() {
    Context ctx = {0};
    ctx.current_token = tokenize("-3*4");

    Node* node = mul(&ctx);

    mu_assert("Node kind should be ND_MUL", node->kind == ND_MUL);
    mu_assert("Right value should be 4", node->rhs->val == 4);
    mu_assert("Left node kind should be ND_SUB", node->lhs->kind == ND_SUB);
    mu_assert("Left-left value should be 0", node->lhs->lhs->val == 0);
    mu_assert("Left-right value should be 3", node->lhs->rhs->val == 3);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_primary_num() {
    Context ctx = {0};
    ctx.current_token = tokenize("42");

    Node* node = primary(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Token should be EOF", ctx.current_token->kind == TK_EOF);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_primary_paren() {
    Context ctx = {0};
    ctx.current_token = tokenize("(42)");

    Node* node = primary(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Token should be EOF", ctx.current_token->kind == TK_EOF);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_mul_single_num() {
    Context ctx = {0};
    ctx.current_token = tokenize("5");

    Node* node = mul(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 5", node->val == 5);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_mul_multiply() {
    Context ctx = {0};
    ctx.current_token = tokenize("2*3");

    Node* node = mul(&ctx);

    mu_assert("Node kind should be ND_MUL", node->kind == ND_MUL);
    mu_assert("Left value should be 2", node->lhs->val == 2);
    mu_assert("Right value should be 3", node->rhs->val == 3);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_mul_divide() {
    Context ctx = {0};
    ctx.current_token = tokenize("6/2");

    Node* node = mul(&ctx);

    mu_assert("Node kind should be ND_DIV", node->kind == ND_DIV);
    mu_assert("Left value should be 6", node->lhs->val == 6);
    mu_assert("Right value should be 2", node->rhs->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_mul_chain() {
    Context ctx = {0};
    ctx.current_token = tokenize("2*3*4");

    Node* node = mul(&ctx);

    mu_assert("Root node kind should be ND_MUL", node->kind == ND_MUL);
    mu_assert("Right value should be 4", node->rhs->val == 4);
    mu_assert("Left node kind should be ND_MUL", node->lhs->kind == ND_MUL);
    mu_assert("Left-left value should be 2", node->lhs->lhs->val == 2);
    mu_assert("Left-right value should be 3", node->lhs->rhs->val == 3);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expr_single_num() {
    Context ctx = {0};
    ctx.current_token = tokenize("42");

    Node* node = expr(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expr_add() {
    Context ctx = {0};
    ctx.current_token = tokenize("1+2");

    Node* node = expr(&ctx);

    mu_assert("Node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expr_subtract() {
    Context ctx = {0};
    ctx.current_token = tokenize("5-3");

    Node* node = expr(&ctx);

    mu_assert("Node kind should be ND_SUB", node->kind == ND_SUB);
    mu_assert("Left value should be 5", node->lhs->val == 5);
    mu_assert("Right value should be 3", node->rhs->val == 3);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expr_add_sub_chain() {
    Context ctx = {0};
    ctx.current_token = tokenize("1-2+3");

    Node* node = expr(&ctx);

    mu_assert("Root node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Right value should be 3", node->rhs->val == 3);
    mu_assert("Left node kind should be ND_SUB", node->lhs->kind == ND_SUB);
    mu_assert("Left-left value should be 1", node->lhs->lhs->val == 1);
    mu_assert("Left-right value should be 2", node->lhs->rhs->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expr_precedence() {
    Context ctx = {0};
    ctx.current_token = tokenize("1+2*3");

    Node* node = expr(&ctx);

    mu_assert("Root node kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right node kind should be ND_MUL", node->rhs->kind == ND_MUL);
    mu_assert("Right-left value should be 2", node->rhs->lhs->val == 2);
    mu_assert("Right-right value should be 3", node->rhs->rhs->val == 3);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expr_complex_precedence() {
    Context ctx = {0};
    ctx.current_token = tokenize("1+2*3-4");

    Node* node = expr(&ctx);

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
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_relational_lt() {
    Context ctx = {0};
    ctx.current_token = tokenize("1<2");

    Node* node = relational(&ctx);

    mu_assert("Node kind should be ND_LT", node->kind == ND_LT);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_relational_le() {
    Context ctx = {0};
    ctx.current_token = tokenize("1<=2");

    Node* node = relational(&ctx);

    mu_assert("Node kind should be ND_LE", node->kind == ND_LE);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_relational_gt() {
    Context ctx = {0};
    ctx.current_token = tokenize("1>2");

    Node* node = relational(&ctx);

    mu_assert("Node kind should be ND_GT", node->kind == ND_GT);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_relational_ge() {
    Context ctx = {0};
    ctx.current_token = tokenize("1>=2");

    Node* node = relational(&ctx);

    mu_assert("Node kind should be ND_GE", node->kind == ND_GE);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_equality_eq() {
    Context ctx = {0};
    ctx.current_token = tokenize("1==2");

    Node* node = equality(&ctx);

    mu_assert("Node kind should be ND_EQ", node->kind == ND_EQ);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_equality_ne() {
    Context ctx = {0};
    ctx.current_token = tokenize("1!=2");

    Node* node = equality(&ctx);

    mu_assert("Node kind should be ND_NE", node->kind == ND_NE);
    mu_assert("Left value should be 1", node->lhs->val == 1);
    mu_assert("Right value should be 2", node->rhs->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expr_combined_precedence() {
    Context ctx = {0};
    ctx.current_token = tokenize("1+2==3");

    Node* node = expr(&ctx);

    mu_assert("Root node kind should be ND_EQ", node->kind == ND_EQ);
    mu_assert("Left node kind should be ND_ADD", node->lhs->kind == ND_ADD);
    mu_assert("Left-left value should be 1", node->lhs->lhs->val == 1);
    mu_assert("Left-right value should be 2", node->lhs->rhs->val == 2);
    mu_assert("Right value should be 3", node->rhs->val == 3);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_new_node_ident() {
    Node* node = new_node_ident("a");

    mu_assert("Node kind should be ND_LVAR", node->kind == ND_LVAR);
    mu_assert("Node val should be 0", node->val == 0);
    mu_assert("Node lhs should be NULL", node->lhs == NULL);
    mu_assert("Node rhs should be NULL", node->rhs == NULL);

    free_ast(node);
    return NULL;
}

char* test_new_node_ident_b() {
    Node* node = new_node_ident("b");

    mu_assert("Node kind should be ND_LVAR", node->kind == ND_LVAR);
    mu_assert("Node val should be 1", node->val == 1);

    free_ast(node);
    return NULL;
}

char* test_primary_ident() {
    Context ctx = {0};
    ctx.current_token = tokenize("a");

    Node* node = primary(&ctx);

    mu_assert("Node kind should be ND_LVAR", node->kind == ND_LVAR);
    mu_assert("Node val should be 0", node->val == 0);
    mu_assert("Token should be EOF", ctx.current_token->kind == TK_EOF);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_assign() {
    Context ctx = {0};
    ctx.current_token = tokenize("a=42");

    Node* node = assign(&ctx);

    mu_assert("Node kind should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Left val should be 0", node->lhs->val == 0);
    mu_assert("Right node kind should be ND_NUM", node->rhs->kind == ND_NUM);
    mu_assert("Right value should be 42", node->rhs->val == 42);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_assign_chain() {
    Context ctx = {0};
    ctx.current_token = tokenize("a=b=5");

    Node* node = assign(&ctx);

    mu_assert("Root node kind should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Left val should be 0", node->lhs->val == 0);
    mu_assert("Right node kind should be ND_ASSIGN",
              node->rhs->kind == ND_ASSIGN);
    mu_assert("Right-left val should be 1", node->rhs->lhs->val == 1);
    mu_assert("Right-right value should be 5", node->rhs->rhs->val == 5);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt() {
    Context ctx = {0};
    ctx.current_token = tokenize("42;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Token should be EOF", ctx.current_token->kind == TK_EOF);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_assign() {
    Context ctx = {0};
    ctx.current_token = tokenize("a=5;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Right node kind should be ND_NUM", node->rhs->kind == ND_NUM);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_program_single_stmt() {
    Context ctx = {0};
    ctx.current_token = tokenize("42;");

    program(&ctx);

    mu_assert("ctx.code[0] should not be NULL", ctx.code[0] != NULL);
    mu_assert("ctx.code[0] kind should be ND_NUM", ctx.code[0]->kind == ND_NUM);
    mu_assert("ctx.code[0] value should be 42", ctx.code[0]->val == 42);
    mu_assert("ctx.code[1] should be NULL", ctx.code[1] == NULL);
    free_ast(ctx.code[0]);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_program_multiple_stmts() {
    Context ctx = {0};
    ctx.current_token = tokenize("1;2;3;");

    program(&ctx);

    mu_assert("ctx.code[0] should not be NULL", ctx.code[0] != NULL);
    mu_assert("ctx.code[0] kind should be ND_NUM", ctx.code[0]->kind == ND_NUM);
    mu_assert("ctx.code[0] value should be 1", ctx.code[0]->val == 1);
    mu_assert("ctx.code[1] should not be NULL", ctx.code[1] != NULL);
    mu_assert("ctx.code[1] kind should be ND_NUM", ctx.code[1]->kind == ND_NUM);
    mu_assert("ctx.code[1] value should be 2", ctx.code[1]->val == 2);
    mu_assert("ctx.code[2] should not be NULL", ctx.code[2] != NULL);
    mu_assert("ctx.code[2] kind should be ND_NUM", ctx.code[2]->kind == ND_NUM);
    mu_assert("ctx.code[2] value should be 3", ctx.code[2]->val == 3);
    mu_assert("ctx.code[3] should be NULL", ctx.code[3] == NULL);
    free_ast(ctx.code[0]);
    free_ast(ctx.code[1]);
    free_ast(ctx.code[2]);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_program_assign_stmts() {
    Context ctx = {0};
    ctx.current_token = tokenize("a=1;b=2;");

    program(&ctx);

    mu_assert("ctx.code[0] should not be NULL", ctx.code[0] != NULL);
    mu_assert("ctx.code[0] kind should be ND_ASSIGN",
              ctx.code[0]->kind == ND_ASSIGN);
    mu_assert("ctx.code[1] should not be NULL", ctx.code[1] != NULL);
    mu_assert("ctx.code[1] kind should be ND_ASSIGN",
              ctx.code[1]->kind == ND_ASSIGN);
    mu_assert("ctx.code[2] should be NULL", ctx.code[2] == NULL);
    free_ast(ctx.code[0]);
    free_ast(ctx.code[1]);
    free_tokens(ctx.current_token);
    return NULL;
}
