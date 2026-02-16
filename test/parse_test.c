#include "parse_test.h"
#include "../src/lex.h"
#include "test_common.h"
#include <stdio.h>
#include <string.h>

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
    Context ctx = {0};
    Token head;
    head.next = NULL;

    Token* tok = new_token(TK_IDENT, &head, "a", 1);
    Node* node = new_node_ident(&ctx, tok);

    mu_assert("Node kind should be ND_LVAR", node->kind == ND_LVAR);
    mu_assert("Node val should be 0", node->val == 0);
    mu_assert("Node lhs should be NULL", node->lhs == NULL);
    mu_assert("Node rhs should be NULL", node->rhs == NULL);

    free_ast(node);
    free_tokens(head.next);
    return NULL;
}

char* test_new_node_ident_abc() {
    Context ctx = {0};
    Token head;
    head.next = NULL;

    Token* tok = new_token(TK_IDENT, &head, "a1", 2);
    Node* node = new_node_ident(&ctx, tok);
    Token* tok2 = new_token(TK_IDENT, tok, "a2", 2);
    Node* node2 = new_node_ident(&ctx, tok2);
    Token* tok3 = new_token(TK_IDENT, tok2, "a3", 2);
    Node* node3 = new_node_ident(&ctx, tok3);

    printf("node2 value %d\n", node2->val);
    mu_assert("Node2 kind should be ND_LVAR", node2->kind == ND_LVAR);
    mu_assert("Node val should be 0", node->val == 0);
    mu_assert("Node2 val should be 1", node2->val == 1);
    mu_assert("Node3 val should be 2", node3->val == 2);
    mu_assert("Node2 lhs should be NULL", node2->lhs == NULL);
    mu_assert("Node2 rhs should be NULL", node2->rhs == NULL);

    free_ast(node);
    free_ast(node2);
    free_ast(node3);
    free_tokens(head.next);
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

char* test_stmt_return_num() {
    Context ctx = {0};
    ctx.current_token = tokenize("return 42;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_RETURN", node->kind == ND_RETURN);
    mu_assert("Left node kind should be ND_NUM", node->lhs->kind == ND_NUM);
    mu_assert("Left node value should be 42", node->lhs->val == 42);
    mu_assert("Right node should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_return_ident() {
    Context ctx = {0};
    ctx.current_token = tokenize("return a;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_RETURN", node->kind == ND_RETURN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Right node should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_return_expr() {
    Context ctx = {0};
    ctx.current_token = tokenize("return a + b;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_RETURN", node->kind == ND_RETURN);
    mu_assert("Left node kind should be ND_ADD", node->lhs->kind == ND_ADD);
    mu_assert("Left-left node kind should be ND_LVAR",
              node->lhs->lhs->kind == ND_LVAR);
    mu_assert("Left-right node kind should be ND_LVAR",
              node->lhs->rhs->kind == ND_LVAR);
    mu_assert("Right node should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_if() {
    Context ctx = {0};
    ctx.current_token = tokenize("if (a) return b;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_IF", node->kind == ND_IF);
    mu_assert("Condition kind should be ND_LVAR", node->cond->kind == ND_LVAR);
    mu_assert("Then kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    mu_assert("Then lhs kind should be ND_LVAR",
              node->lhs->lhs->kind == ND_LVAR);
    mu_assert("Else should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_if_else() {
    Context ctx = {0};
    ctx.current_token = tokenize("if (a) return b; else return c;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_IF", node->kind == ND_IF);
    mu_assert("Condition kind should be ND_LVAR", node->cond->kind == ND_LVAR);
    mu_assert("Then kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    mu_assert("Else kind should be ND_RETURN", node->rhs->kind == ND_RETURN);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_if_with_block() {
    Context ctx = {0};
    ctx.current_token = tokenize("if (a) { return b; }");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_IF", node->kind == ND_IF);
    mu_assert("Condition kind should be ND_LVAR", node->cond->kind == ND_LVAR);
    mu_assert("Then kind should be ND_BLOCK", node->lhs->kind == ND_BLOCK);
    mu_assert("Then first stmt kind should be ND_RETURN",
              node->lhs->lhs->kind == ND_RETURN);
    mu_assert("Else should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_if_complex_cond() {
    Context ctx = {0};
    ctx.current_token = tokenize("if (a == b) return c;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_IF", node->kind == ND_IF);
    mu_assert("Condition kind should be ND_EQ", node->cond->kind == ND_EQ);
    mu_assert("Then kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_while() {
    Context ctx = {0};
    ctx.current_token = tokenize("while (a) return b;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_WHILE", node->kind == ND_WHILE);
    mu_assert("Condition kind should be ND_LVAR", node->cond->kind == ND_LVAR);
    mu_assert("Body kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    mu_assert("Body lhs kind should be ND_LVAR",
              node->lhs->lhs->kind == ND_LVAR);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_while_complex_cond() {
    Context ctx = {0};
    ctx.current_token = tokenize("while (a < b) a = a + 1;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_WHILE", node->kind == ND_WHILE);
    mu_assert("Condition kind should be ND_LT", node->cond->kind == ND_LT);
    mu_assert("Body kind should be ND_ASSIGN", node->lhs->kind == ND_ASSIGN);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_for() {
    Context ctx = {0};
    ctx.current_token = tokenize("for (a = 0; a < 10; a = a + 1) return b;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_FOR", node->kind == ND_FOR);
    mu_assert("Init kind should be ND_ASSIGN", node->init->kind == ND_ASSIGN);
    mu_assert("Condition kind should be ND_LT", node->cond->kind == ND_LT);
    mu_assert("Inc kind should be ND_ASSIGN", node->rhs->kind == ND_ASSIGN);
    mu_assert("Body kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_for_no_init() {
    Context ctx = {0};
    ctx.current_token = tokenize("for (; a < 10;) return b;");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_FOR", node->kind == ND_FOR);
    mu_assert("Init should be NULL", node->init == NULL);
    mu_assert("Condition kind should be ND_LT", node->cond->kind == ND_LT);
    mu_assert("Inc should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_program_single_stmt() {
    Context ctx = {0};
    ctx.current_token = tokenize("main() { 42; }");

    program(&ctx);

    mu_assert("ctx.code[0] should not be NULL", ctx.code[0] != NULL);
    mu_assert("ctx.code[0] kind should be ND_FUNCTION",
              ctx.code[0]->kind == ND_FUNCTION);
    mu_assert("ctx.code[0] name should be 'main'",
              ctx.code[0]->tok->len == 4 &&
                  strncmp(ctx.code[0]->tok->str, "main", 4) == 0);
    mu_assert("ctx.code[1] should be NULL", ctx.code[1] == NULL);
    free_ast(ctx.code[0]);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_program_multiple_stmts() {
    Context ctx = {0};
    ctx.current_token = tokenize("main() { 1; 2; 3; }");

    program(&ctx);

    mu_assert("ctx.code[0] should not be NULL", ctx.code[0] != NULL);
    mu_assert("ctx.code[0] kind should be ND_FUNCTION",
              ctx.code[0]->kind == ND_FUNCTION);
    mu_assert("ctx.code[0] body first stmt value should be 1",
              ctx.code[0]->lhs->val == 1);
    mu_assert("ctx.code[0] body second stmt value should be 2",
              ctx.code[0]->lhs->next->val == 2);
    mu_assert("ctx.code[0] body third stmt value should be 3",
              ctx.code[0]->lhs->next->next->val == 3);
    mu_assert("ctx.code[1] should be NULL", ctx.code[1] == NULL);
    free_ast(ctx.code[0]);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_program_assign_stmts() {
    Context ctx = {0};
    ctx.current_token = tokenize("main() { a=1; b=2; }");

    program(&ctx);

    mu_assert("ctx.code[0] should not be NULL", ctx.code[0] != NULL);
    mu_assert("ctx.code[0] kind should be ND_FUNCTION",
              ctx.code[0]->kind == ND_FUNCTION);
    mu_assert("ctx.code[0] body first stmt kind should be ND_ASSIGN",
              ctx.code[0]->lhs->kind == ND_ASSIGN);
    mu_assert("ctx.code[0] body second stmt kind should be ND_ASSIGN",
              ctx.code[0]->lhs->next->kind == ND_ASSIGN);
    mu_assert("ctx.code[1] should be NULL", ctx.code[1] == NULL);
    free_ast(ctx.code[0]);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_call() {
    Context ctx = {0};
    ctx.current_token = tokenize("foo();");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_CALL", node->kind == ND_CALL);
    mu_assert("tok should not be NULL", node->tok != NULL);
    mu_assert("LHS (args) should be NULL", node->lhs == NULL);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_call_with_args() {
    Context ctx = {0};
    ctx.current_token = tokenize("foo(40, 2);");

    Node* node = stmt(&ctx);

    mu_assert("Node kind should be ND_CALL", node->kind == ND_CALL);
    mu_assert("tok should not be NULL", node->tok != NULL);
    mu_assert("LHS (args) should not be NULL", node->lhs != NULL);
    mu_assert("First arg should be ND_NUM", node->lhs->kind == ND_NUM);
    mu_assert("First arg value should be 40", node->lhs->val == 40);
    mu_assert("Second arg should exist", node->lhs->next != NULL);
    mu_assert("Second arg should be ND_NUM", node->lhs->next->kind == ND_NUM);
    mu_assert("Second arg value should be 2", node->lhs->next->val == 2);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_function_simple() {
    Context ctx = {0};
    ctx.current_token = tokenize("main() { return 42; }");

    Node* node = function(&ctx);

    mu_assert("Node kind should be ND_FUNCTION", node->kind == ND_FUNCTION);
    mu_assert("Function name should be 'main'",
              node->tok->len == 4 && strncmp(node->tok->str, "main", 4) == 0);
    mu_assert("Body should not be NULL", node->lhs != NULL);
    mu_assert("First stmt kind should be ND_RETURN",
              node->lhs->kind == ND_RETURN);
    mu_assert("First stmt lhs value should be 42", node->lhs->lhs->val == 42);
    mu_assert("Token should be EOF", ctx.current_token->kind == TK_EOF);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_function_multiple_stmts() {
    Context ctx = {0};
    ctx.current_token = tokenize("main() { a=1; return a; }");

    Node* node = function(&ctx);

    mu_assert("Node kind should be ND_FUNCTION", node->kind == ND_FUNCTION);
    mu_assert("Body should not be NULL", node->lhs != NULL);
    mu_assert("First stmt kind should be ND_ASSIGN",
              node->lhs->kind == ND_ASSIGN);
    mu_assert("Second stmt should exist", node->lhs->next != NULL);
    mu_assert("Second stmt kind should be ND_RETURN",
              node->lhs->next->kind == ND_RETURN);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_program_single_function() {
    Context ctx = {0};
    ctx.current_token = tokenize("main() { return 42; }");

    program(&ctx);

    mu_assert("ctx.code[0] should not be NULL", ctx.code[0] != NULL);
    mu_assert("ctx.code[0] kind should be ND_FUNCTION",
              ctx.code[0]->kind == ND_FUNCTION);
    mu_assert("ctx.code[0] name should be 'main'",
              ctx.code[0]->tok->len == 4 &&
                  strncmp(ctx.code[0]->tok->str, "main", 4) == 0);
    mu_assert("ctx.code[1] should be NULL", ctx.code[1] == NULL);
    free_ast(ctx.code[0]);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_program_multiple_functions() {
    Context ctx = {0};
    ctx.current_token = tokenize("foo() { return 1; } main() { return 42; }");

    program(&ctx);

    mu_assert("ctx.code[0] should not be NULL", ctx.code[0] != NULL);
    mu_assert("ctx.code[0] kind should be ND_FUNCTION",
              ctx.code[0]->kind == ND_FUNCTION);
    mu_assert("ctx.code[0] name should be 'foo'",
              ctx.code[0]->tok->len == 3 &&
                  strncmp(ctx.code[0]->tok->str, "foo", 3) == 0);
    mu_assert("ctx.code[1] should not be NULL", ctx.code[1] != NULL);
    mu_assert("ctx.code[1] kind should be ND_FUNCTION",
              ctx.code[1]->kind == ND_FUNCTION);
    mu_assert("ctx.code[1] name should be 'main'",
              ctx.code[1]->tok->len == 4 &&
                  strncmp(ctx.code[1]->tok->str, "main", 4) == 0);
    free_ast(ctx.code[0]);
    free_ast(ctx.code[1]);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_function_with_block() {
    Context ctx = {0};
    ctx.current_token = tokenize("main() { if (1) { return 42; } }");

    Node* node = function(&ctx);

    mu_assert("Node kind should be ND_FUNCTION", node->kind == ND_FUNCTION);
    mu_assert("Body should not be NULL", node->lhs != NULL);
    mu_assert("First stmt kind should be ND_IF", node->lhs->kind == ND_IF);
    mu_assert("If body kind should be ND_BLOCK",
              node->lhs->lhs->kind == ND_BLOCK);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_function_with_params() {
    Context ctx = {0};
    ctx.current_token = tokenize("foo(a, b) { return a + b; }");

    Node* node = function(&ctx);

    mu_assert("Node kind should be ND_FUNCTION", node->kind == ND_FUNCTION);
    mu_assert("Function name should be 'foo'",
              node->tok->len == 3 && strncmp(node->tok->str, "foo", 3) == 0);
    mu_assert("Params should not be NULL", node->rhs != NULL);
    mu_assert("First param kind should be ND_LVAR", node->rhs->kind == ND_LVAR);
    mu_assert("First param offset should be 0", node->rhs->val == 0);
    mu_assert("Second param should exist", node->rhs->next != NULL);
    mu_assert("Second param kind should be ND_LVAR",
              node->rhs->next->kind == ND_LVAR);
    mu_assert("Second param offset should be 1", node->rhs->next->val == 1);
    mu_assert("Body should not be NULL", node->lhs != NULL);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_function_with_single_param() {
    Context ctx = {0};
    ctx.current_token = tokenize("bar(x) { return x; }");

    Node* node = function(&ctx);

    mu_assert("Node kind should be ND_FUNCTION", node->kind == ND_FUNCTION);
    mu_assert("Function name should be 'bar'",
              node->tok->len == 3 && strncmp(node->tok->str, "bar", 3) == 0);
    mu_assert("Params should not be NULL", node->rhs != NULL);
    mu_assert("Param kind should be ND_LVAR", node->rhs->kind == ND_LVAR);
    mu_assert("Param offset should be 0", node->rhs->val == 0);
    mu_assert("No second param", node->rhs->next == NULL);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}
