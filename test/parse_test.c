#include "parse_test.h"
#include "../src/lex.h"
#include "../src/variable.h"
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

    Node* node = parse_unary(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_unary_plus() {
    Context ctx = {0};
    ctx.current_token = tokenize("+42");

    Node* node = parse_unary(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_unary_minus() {
    Context ctx = {0};
    ctx.current_token = tokenize("-5");

    Node* node = parse_unary(&ctx);

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

    Node* node = parse_expr(&ctx);

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

    Node* node = parse_mul(&ctx);

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

    Node* node = parse_primary(&ctx);

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

    Node* node = parse_primary(&ctx);

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

    Node* node = parse_mul(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 5", node->val == 5);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_mul_multiply() {
    Context ctx = {0};
    ctx.current_token = tokenize("2*3");

    Node* node = parse_mul(&ctx);

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

    Node* node = parse_mul(&ctx);

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

    Node* node = parse_mul(&ctx);

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

    Node* node = parse_expr(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_expr_add() {
    Context ctx = {0};
    ctx.current_token = tokenize("1+2");

    Node* node = parse_expr(&ctx);

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

    Node* node = parse_expr(&ctx);

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

    Node* node = parse_expr(&ctx);

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

    Node* node = parse_expr(&ctx);

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

    Node* node = parse_expr(&ctx);

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

    Node* node = parse_relational(&ctx);

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

    Node* node = parse_relational(&ctx);

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

    Node* node = parse_relational(&ctx);

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

    Node* node = parse_relational(&ctx);

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

    Node* node = parse_equality(&ctx);

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

    Node* node = parse_equality(&ctx);

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

    Node* node = parse_expr(&ctx);

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

    // First declare the variable
    Token* tok = new_token(TK_IDENT, &head, "a", 1);
    add_lvar(&ctx, tok, new_type_int());

    // Then use it
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

    // First declare variables
    Token* tok = new_token(TK_IDENT, &head, "a1", 2);
    add_lvar(&ctx, tok, new_type_int());
    Token* tok2 = new_token(TK_IDENT, tok, "a2", 2);
    add_lvar(&ctx, tok2, new_type_int());
    Token* tok3 = new_token(TK_IDENT, tok2, "a3", 2);
    add_lvar(&ctx, tok3, new_type_int());

    // Then use them
    Node* node = new_node_ident(&ctx, tok);
    Node* node2 = new_node_ident(&ctx, tok2);
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
    // First declare the variable
    Token* tok = tokenize("a");
    add_lvar(&ctx, tok, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_primary(&ctx);

    mu_assert("Node kind should be ND_LVAR", node->kind == ND_LVAR);
    mu_assert("Node val should be 0", node->val == 0);
    mu_assert("Token should be EOF", ctx.current_token->kind == TK_EOF);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_assign() {
    Context ctx = {0};
    // First declare variable 'a'
    Token* tok = tokenize("a=42");
    Token* tok_a = tok; // 'a' token
    add_lvar(&ctx, tok_a, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_assign(&ctx);

    mu_assert("Node kind should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Left val should be 0", node->lhs->val == 0);
    mu_assert("Right node kind should be ND_NUM", node->rhs->kind == ND_NUM);
    mu_assert("Right value should be 42", node->rhs->val == 42);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_assign_chain() {
    Context ctx = {0};
    // First declare variables 'a' and 'b'
    Token* tok = tokenize("a=b=5");
    Token* tok_a = tok;             // 'a' token
    Token* tok_b = tok->next->next; // 'b' token (skip '=')
    add_lvar(&ctx, tok_a, new_type_int());
    add_lvar(&ctx, tok_b, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_assign(&ctx);

    mu_assert("Root node kind should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Left val should be 0", node->lhs->val == 0);
    mu_assert("Right node kind should be ND_ASSIGN",
              node->rhs->kind == ND_ASSIGN);
    mu_assert("Right-left val should be 1", node->rhs->lhs->val == 1);
    mu_assert("Right-right value should be 5", node->rhs->rhs->val == 5);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt() {
    Context ctx = {0};
    ctx.current_token = tokenize("42;");

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_NUM", node->kind == ND_NUM);
    mu_assert("Node value should be 42", node->val == 42);
    mu_assert("Token should be EOF", ctx.current_token->kind == TK_EOF);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_assign() {
    Context ctx = {0};
    // First declare variable 'a'
    Token* tok = tokenize("a=5;");
    Token* tok_a = tok; // 'a' token
    add_lvar(&ctx, tok_a, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Right node kind should be ND_NUM", node->rhs->kind == ND_NUM);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_return_num() {
    Context ctx = {0};
    ctx.current_token = tokenize("return 42;");

    Node* node = parse_stmt(&ctx);

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
    // First declare variable 'a'
    Token* tok = tokenize("return a;");
    Token* tok_a = tok->next; // 'a' token
    add_lvar(&ctx, tok_a, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_RETURN", node->kind == ND_RETURN);
    mu_assert("Left node kind should be ND_LVAR", node->lhs->kind == ND_LVAR);
    mu_assert("Right node should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_return_expr() {
    Context ctx = {0};
    // First declare variables 'a' and 'b'
    Token* tok = tokenize("return a + b;");
    Token* tok_a = tok->next; // 'a' token (after 'return')
    Token* tok_b =
        tok->next->next->next; // 'b' token (after 'return', 'a', '+')
    add_lvar(&ctx, tok_a, new_type_int());
    add_lvar(&ctx, tok_b, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_RETURN", node->kind == ND_RETURN);
    mu_assert("Left node kind should be ND_ADD", node->lhs->kind == ND_ADD);
    mu_assert("Left-left node kind should be ND_LVAR",
              node->lhs->lhs->kind == ND_LVAR);
    mu_assert("Left-right node kind should be ND_LVAR",
              node->lhs->rhs->kind == ND_LVAR);
    mu_assert("Right node should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_if() {
    Context ctx = {0};
    // First declare variables 'a' and 'b'
    Token* tok = tokenize("if (a) return b;");
    Token* tok_a = tok->next->next; // 'a' token (after 'if', '(')
    Token* tok_b =
        tok->next->next->next->next
            ->next; // 'b' token (after 'if', '(', 'a', ')', 'return')
    add_lvar(&ctx, tok_a, new_type_int());
    add_lvar(&ctx, tok_b, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_IF", node->kind == ND_IF);
    mu_assert("Condition kind should be ND_LVAR", node->cond->kind == ND_LVAR);
    mu_assert("Then kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    mu_assert("Then lhs kind should be ND_LVAR",
              node->lhs->lhs->kind == ND_LVAR);
    mu_assert("Else should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_if_else() {
    Context ctx = {0};
    Token* tok = tokenize("if (a) return b; else return c;");
    // Declare variables 'a', 'b', 'c'
    // Tokens: if, (, a, ), return, b, ;, else, return, c, ;
    Token* tok_a = tok->next->next;                   // 'a'
    Token* tok_b = tok->next->next->next->next->next; // 'b'
    Token* tok_c =
        tok->next->next->next->next->next->next->next->next->next; // 'c'
    add_lvar(&ctx, tok_a, new_type_int());
    add_lvar(&ctx, tok_b, new_type_int());
    add_lvar(&ctx, tok_c, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_IF", node->kind == ND_IF);
    mu_assert("Condition kind should be ND_LVAR", node->cond->kind == ND_LVAR);
    mu_assert("Then kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    mu_assert("Else kind should be ND_RETURN", node->rhs->kind == ND_RETURN);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_if_with_block() {
    Context ctx = {0};
    Token* tok = tokenize("if (a) { return b; }");
    Token* tok_a = tok->next->next;                         // 'a'
    Token* tok_b = tok->next->next->next->next->next->next; // 'b'
    add_lvar(&ctx, tok_a, new_type_int());
    add_lvar(&ctx, tok_b, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_IF", node->kind == ND_IF);
    mu_assert("Condition kind should be ND_LVAR", node->cond->kind == ND_LVAR);
    mu_assert("Then kind should be ND_BLOCK", node->lhs->kind == ND_BLOCK);
    mu_assert("Then first stmt kind should be ND_RETURN",
              node->lhs->lhs->kind == ND_RETURN);
    mu_assert("Else should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_if_complex_cond() {
    Context ctx = {0};
    Token* tok = tokenize("if (a == b) return c;");
    Token* tok_a = tok->next->next;                               // 'a'
    Token* tok_b = tok->next->next->next->next;                   // 'b'
    Token* tok_c = tok->next->next->next->next->next->next->next; // 'c'
    add_lvar(&ctx, tok_a, new_type_int());
    add_lvar(&ctx, tok_b, new_type_int());
    add_lvar(&ctx, tok_c, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_IF", node->kind == ND_IF);
    mu_assert("Condition kind should be ND_EQ", node->cond->kind == ND_EQ);
    mu_assert("Then kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_while() {
    Context ctx = {0};
    Token* tok = tokenize("while (a) return b;");
    Token* tok_a = tok->next->next;                   // 'a'
    Token* tok_b = tok->next->next->next->next->next; // 'b'
    add_lvar(&ctx, tok_a, new_type_int());
    add_lvar(&ctx, tok_b, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_WHILE", node->kind == ND_WHILE);
    mu_assert("Condition kind should be ND_LVAR", node->cond->kind == ND_LVAR);
    mu_assert("Body kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    mu_assert("Body lhs kind should be ND_LVAR",
              node->lhs->lhs->kind == ND_LVAR);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_while_complex_cond() {
    Context ctx = {0};
    Token* tok = tokenize("while (a < b) a = a + 1;");
    Token* tok_a = tok->next->next;             // 'a'
    Token* tok_b = tok->next->next->next->next; // 'b'
    add_lvar(&ctx, tok_a, new_type_int());
    add_lvar(&ctx, tok_b, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_WHILE", node->kind == ND_WHILE);
    mu_assert("Condition kind should be ND_LT", node->cond->kind == ND_LT);
    mu_assert("Body kind should be ND_ASSIGN", node->lhs->kind == ND_ASSIGN);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_for() {
    Context ctx = {0};
    Token* tok = tokenize("for (a = 0; a < 10; a = a + 1) return 42;");
    Token* tok_a = tok->next->next; // 'a'
    add_lvar(&ctx, tok_a, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_FOR", node->kind == ND_FOR);
    mu_assert("Init kind should be ND_ASSIGN", node->init->kind == ND_ASSIGN);
    mu_assert("Condition kind should be ND_LT", node->cond->kind == ND_LT);
    mu_assert("Inc kind should be ND_ASSIGN", node->rhs->kind == ND_ASSIGN);
    mu_assert("Body kind should be ND_RETURN", node->lhs->kind == ND_RETURN);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_stmt_for_no_init() {
    Context ctx = {0};
    Token* tok = tokenize("for (; a < 10;) return b;");
    // Tokens: for, (, ;, a, <, 10, ;, ), return, b, ;
    Token* tok_a = tok->next->next->next; // 'a' (after 'for', '(', ';')
    Token* tok_b =
        tok->next->next->next->next->next->next->next->next->next; // 'b'
    add_lvar(&ctx, tok_a, new_type_int());
    add_lvar(&ctx, tok_b, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_FOR", node->kind == ND_FOR);
    mu_assert("Init should be NULL", node->init == NULL);
    mu_assert("Condition kind should be ND_LT", node->cond->kind == ND_LT);
    mu_assert("Inc should be NULL", node->rhs == NULL);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_program_single_stmt() {
    Context ctx = {0};
    ctx.current_token = tokenize("int main() { 42; }");

    parse_program(&ctx);

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
    ctx.current_token = tokenize("int main() { 1; 2; 3; }");

    parse_program(&ctx);

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
    ctx.current_token = tokenize("int main() { int a; int b; a=1; b=2; }");

    parse_program(&ctx);

    mu_assert("ctx.code[0] should not be NULL", ctx.code[0] != NULL);
    mu_assert("ctx.code[0] kind should be ND_FUNCTION",
              ctx.code[0]->kind == ND_FUNCTION);
    mu_assert("ctx.code[0] body first stmt kind should be ND_DECL",
              ctx.code[0]->lhs->kind == ND_DECL);
    mu_assert("ctx.code[0] body second stmt kind should be ND_DECL",
              ctx.code[0]->lhs->next->kind == ND_DECL);
    mu_assert("ctx.code[0] body third stmt kind should be ND_ASSIGN",
              ctx.code[0]->lhs->next->next->kind == ND_ASSIGN);
    mu_assert("ctx.code[0] body fourth stmt kind should be ND_ASSIGN",
              ctx.code[0]->lhs->next->next->next->kind == ND_ASSIGN);
    mu_assert("ctx.code[1] should be NULL", ctx.code[1] == NULL);
    free_ast(ctx.code[0]);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_stmt_call() {
    Context ctx = {0};
    ctx.current_token = tokenize("foo();");

    Node* node = parse_stmt(&ctx);

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

    Node* node = parse_stmt(&ctx);

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
    ctx.current_token = tokenize("int main() { return 42; }");

    Node* node = parse_function(&ctx);

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
    ctx.current_token = tokenize("int main() { int a; a=1; return a; }");

    Node* node = parse_function(&ctx);

    mu_assert("Node kind should be ND_FUNCTION", node->kind == ND_FUNCTION);
    mu_assert("Body should not be NULL", node->lhs != NULL);
    mu_assert("First stmt kind should be ND_DECL", node->lhs->kind == ND_DECL);
    mu_assert("Second stmt should exist", node->lhs->next != NULL);
    mu_assert("Second stmt kind should be ND_ASSIGN",
              node->lhs->next->kind == ND_ASSIGN);
    mu_assert("Third stmt should exist", node->lhs->next->next != NULL);
    mu_assert("Third stmt kind should be ND_RETURN",
              node->lhs->next->next->kind == ND_RETURN);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_program_single_function() {
    Context ctx = {0};
    ctx.current_token = tokenize("int main() { return 42; }");

    parse_program(&ctx);

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
    ctx.current_token =
        tokenize("int foo() { return 1; } int main() { return 42; }");

    parse_program(&ctx);

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
    ctx.current_token = tokenize("int main() { if (1) { return 42; } }");

    Node* node = parse_function(&ctx);

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
    ctx.current_token = tokenize("int foo(int a, int b) { return a + b; }");

    Node* node = parse_function(&ctx);

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
    ctx.current_token = tokenize("int bar(int x) { return x; }");

    Node* node = parse_function(&ctx);

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

char* test_unary_deref() {
    Context ctx = {0};
    Token* tok = tokenize("*p");
    Token* tok_p = tok->next; // 'p'
    // p is a pointer type (int*)
    add_lvar(&ctx, tok_p, new_type_ptr(new_type_int()));
    ctx.current_token = tok;

    Node* node = parse_unary(&ctx);

    mu_assert("Node kind should be ND_DEREF", node->kind == ND_DEREF);
    mu_assert("Operand should be ND_LVAR", node->lhs->kind == ND_LVAR);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_unary_addr() {
    Context ctx = {0};
    Token* tok = tokenize("&x");
    Token* tok_x = tok->next; // 'x'
    add_lvar(&ctx, tok_x, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_unary(&ctx);

    mu_assert("Node kind should be ND_ADDR", node->kind == ND_ADDR);
    mu_assert("Operand should be ND_LVAR", node->lhs->kind == ND_LVAR);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_unary_deref_complex() {
    Context ctx = {0};
    Token* tok = tokenize("**p");
    Token* tok_p = tok->next->next; // 'p' (after '*', '*')
    // p is a pointer to pointer (int**)
    Type* int_ptr = new_type_ptr(new_type_int());
    add_lvar(&ctx, tok_p, new_type_ptr(int_ptr));
    ctx.current_token = tok;

    Node* node = parse_unary(&ctx);

    mu_assert("Node kind should be ND_DEREF", node->kind == ND_DEREF);
    mu_assert("Operand should be ND_DEREF", node->lhs->kind == ND_DEREF);
    mu_assert("Inner operand should be ND_LVAR",
              node->lhs->lhs->kind == ND_LVAR);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_expr_with_deref() {
    Context ctx = {0};
    Token* tok = tokenize("*p + 1");
    Token* tok_p = tok->next; // 'p'
    // p is a pointer type (int*)
    add_lvar(&ctx, tok_p, new_type_ptr(new_type_int()));
    ctx.current_token = tok;

    Node* node = parse_expr(&ctx);

    mu_assert("Root kind should be ND_ADD", node->kind == ND_ADD);
    mu_assert("Left should be ND_DEREF", node->lhs->kind == ND_DEREF);
    mu_assert("Right should be ND_NUM", node->rhs->kind == ND_NUM);
    mu_assert("Right value should be 1", node->rhs->val == 1);
    free_ast(node);
    free_tokens(ctx.current_token);
    return NULL;
}

char* test_parse_add_assign_does_not_share_lhs_node() {
    Context ctx = {0};
    Token* tok = tokenize("a += 1");
    Token* tok_a = tok;
    add_lvar(&ctx, tok_a, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_expr(&ctx);

    mu_assert("node should be assignment", node->kind == ND_ASSIGN);
    mu_assert("assignment rhs should be add", node->rhs->kind == ND_ADD);
    mu_assert("compound assign should not share lhs node pointer",
              node->lhs != node->rhs->lhs);
    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_global_ptr_init_not_treated_as_array() {
    Context ctx = {0};
    Token* tok = tokenize("int gx = 10; int* gpx = &gx;");
    ctx.current_token = tok;

    parse_program(&ctx);

    mu_assert("first global should exist", ctx.code[0] != NULL);
    mu_assert("second global should exist", ctx.code[1] != NULL);
    mu_assert("second global should be ND_GVAR", ctx.code[1]->kind == ND_GVAR);
    mu_assert("gpx type should be PTR", ctx.code[1]->type->ty == PTR);
    mu_assert("gpx should not be treated as array",
              ctx.code[1]->type->array_size == 0);
    mu_assert("gpx initializer should be address-of",
              ctx.code[1]->init && ctx.code[1]->init->kind == ND_ADDR);

    free_ast(ctx.code[0]);
    free_ast(ctx.code[1]);
    free_tokens(tok);
    return NULL;
}

char* test_scope_depth_is_context_local() {
    Context ctx1 = {0};
    Context ctx2 = {0};
    Token* tok = tokenize("x");

    reset_scope(&ctx1);
    enter_scope(&ctx1);
    add_lvar(&ctx1, tok, new_type_int());

    // Another context resets its own scope; this must not affect ctx1.
    reset_scope(&ctx2);
    add_lvar(&ctx2, tok, new_type_int());

    mu_assert("ctx1 local should still be visible in ctx1",
              find_lvar(&ctx1, tok) != NULL);

    leave_scope(&ctx1);
    free_tokens(tok);
    return NULL;
}

char* test_parse_double() {
    Context ctx = {0};
    Token* tok = tokenize("double x = 1.23;");
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_DECL", node->kind == ND_DECL);
    mu_assert("Node type should be DOUBLE", node->type->ty == DOUBLE);
    mu_assert("Node initializer should exist", node->init != NULL);
    mu_assert("Node initializer kind should be ND_FNUM",
              node->init->kind == ND_FNUM);
    mu_assert("Node initializer fval should be 1.23", node->init->fval == 1.23);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_float() {
    Context ctx = {0};
    Token* tok = tokenize("float y = 1.23f;");
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_DECL", node->kind == ND_DECL);
    mu_assert("Node type should be FLOAT", node->type->ty == FLOAT);
    mu_assert("Node initializer should exist", node->init != NULL);
    mu_assert("Node initializer kind should be ND_FNUM",
              node->init->kind == ND_FNUM);
    mu_assert("Node initializer fval should be 1.23", node->init->fval == 1.23);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_do_while() {
    Context ctx = {0};
    Token* tok = tokenize("do { a = a + 1; } while (a < 10);");
    // Declare 'a'
    add_lvar(&ctx, tok->next->next, new_type_int());
    ctx.current_token = tok;

    Node* node = parse_stmt(&ctx);

    mu_assert("Node kind should be ND_WHILE", node->kind == ND_WHILE);
    mu_assert("Node should be do-while", node->is_do_while);
    mu_assert("Condition kind should be ND_LT", node->cond->kind == ND_LT);
    mu_assert("Body kind should be ND_BLOCK", node->lhs->kind == ND_BLOCK);
    // Note: I might need a new NodeKind or a flag for do-while to distinguish
    // it from while in codegen. In C, do-while always executes at least once.
    // I'll add a flag to Node.

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_uac() {
    Context ctx = {0};
    // unsigned int u; int i; u + i
    Token* tok_u = tokenize("unsigned int u;");
    ctx.current_token = tok_u;
    Node* decl_u = parse_stmt(&ctx);

    Token* tok_i = tokenize("int i;");
    ctx.current_token = tok_i;
    Node* decl_i = parse_stmt(&ctx);

    Token* tok_add = tokenize("u + i;");
    ctx.current_token = tok_add;
    Node* node_add = parse_stmt(&ctx);

    mu_assert("node_add should be ND_ADD", node_add->kind == ND_ADD);
    mu_assert("result of u + i should be unsigned",
              node_add->type->is_unsigned);
    mu_assert("result of u + i should be INT", node_add->type->ty == INT);

    // long l; unsigned int u; l + u -> long (signed)
    Token* tok_l = tokenize("long l;");
    ctx.current_token = tok_l;
    Node* decl_l = parse_stmt(&ctx);

    Token* tok_add2 = tokenize("l + u;");
    ctx.current_token = tok_add2;
    Node* node_add2 = parse_stmt(&ctx);

    mu_assert("result of l + u should be long", node_add2->type->ty == LONG);
    mu_assert("result of l + u should be signed",
              !node_add2->type->is_unsigned);

    return NULL;
}

char* test_parse_bitwise() {
    Context ctx = {0};
    // 1 | 2 ^ 3 & 4
    Token* tok = tokenize("1 | 2 ^ 3 & 4;");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    // Should be: (1 | (2 ^ (3 & 4)))
    mu_assert("node should be ND_BITOR", node->kind == ND_BITOR);
    mu_assert("lhs of | should be 1",
              node->lhs->kind == ND_NUM && node->lhs->val == 1);
    mu_assert("rhs of | should be ND_BITXOR", node->rhs->kind == ND_BITXOR);
    mu_assert("lhs of ^ should be 2",
              node->rhs->lhs->kind == ND_NUM && node->rhs->lhs->val == 2);
    mu_assert("rhs of ^ should be ND_BITAND",
              node->rhs->rhs->kind == ND_BITAND);
    mu_assert("lhs of & should be 3", node->rhs->rhs->lhs->kind == ND_NUM &&
                                          node->rhs->rhs->lhs->val == 3);
    mu_assert("rhs of & should be 4", node->rhs->rhs->rhs->kind == ND_NUM &&
                                          node->rhs->rhs->rhs->val == 4);

    // Unary bitwise NOT: ~1
    Token* tok_not = tokenize("~1;");
    ctx.current_token = tok_not;
    Node* node_not = parse_stmt(&ctx);
    mu_assert("node_not should be ND_BITNOT", node_not->kind == ND_BITNOT);
    mu_assert("operand of ~ should be 1",
              node_not->lhs->kind == ND_NUM && node_not->lhs->val == 1);

    return NULL;
}

char* test_parse_shift() {
    Context ctx = {0};
    // 1 << 2 + 3
    Token* tok = tokenize("1 << 2 + 3;");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    // Should be: (1 << (2 + 3))
    mu_assert("node should be ND_SHL", node->kind == ND_SHL);
    mu_assert("lhs of << should be 1",
              node->lhs->kind == ND_NUM && node->lhs->val == 1);
    mu_assert("rhs of << should be ND_ADD", node->rhs->kind == ND_ADD);

    // 1 + 2 << 3
    Token* tok2 = tokenize("1 + 2 << 3;");
    ctx.current_token = tok2;
    Node* node2 = parse_stmt(&ctx);
    // Should be: ((1 + 2) << 3)
    mu_assert("node2 should be ND_SHL", node2->kind == ND_SHL);
    mu_assert("lhs of node2 << should be ND_ADD", node2->lhs->kind == ND_ADD);
    mu_assert("rhs of node2 << should be 3",
              node2->rhs->kind == ND_NUM && node2->rhs->val == 3);

    return NULL;
}

char* test_parse_compound_bitwise() {
    Context ctx = {0};
    Token x_tok = {0};
    x_tok.kind = TK_IDENT;
    x_tok.str = "x";
    x_tok.len = 1;
    add_lvar(&ctx, &x_tok, new_type_int());
    // x &= 1
    Token* tok = tokenize("x &= 1;");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    mu_assert("node should be ND_ASSIGN", node->kind == ND_ASSIGN);
    mu_assert("rhs should be ND_BITAND", node->rhs->kind == ND_BITAND);
    mu_assert("lhs of &= should be x", node->lhs->kind == ND_LVAR);

    // x <<= 2
    Token* tok2 = tokenize("x <<= 2;");
    ctx.current_token = tok2;
    Node* node2 = parse_stmt(&ctx);
    mu_assert("node2 should be ND_ASSIGN", node2->kind == ND_ASSIGN);
    mu_assert("rhs of node2 should be ND_SHL", node2->rhs->kind == ND_SHL);

    return NULL;
}

char* test_parse_union_decl() {
    Context ctx = {0};
    Token* tok = tokenize("union { int a; char b; } u;");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    mu_assert("node kind should be ND_DECL", node->kind == ND_DECL);
    mu_assert("decl type should be UNION", node->type->ty == UNION);
    mu_assert("union should have members", node->type->members != NULL);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_bitfield_decl() {
    Context ctx = {0};
    Token* tok = tokenize("struct { unsigned int a:3; unsigned int b:5; } s;");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    mu_assert("node kind should be ND_DECL", node->kind == ND_DECL);
    mu_assert("decl type should be STRUCT", node->type->ty == STRUCT);
    mu_assert("members should exist", node->type->members != NULL);
    mu_assert("first member should be bitfield",
              node->type->members->is_bitfield);
    mu_assert("first width should be 3", node->type->members->bit_width == 3);
    mu_assert("second member should be bitfield",
              node->type->members->next &&
                  node->type->members->next->is_bitfield);
    mu_assert("second width should be 5",
              node->type->members->next->bit_width == 5);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_goto_label() {
    Context ctx = {0};
    Token* tok = tokenize("goto L; L: return 1;");
    ctx.current_token = tok;

    Node* n1 = parse_stmt(&ctx);
    mu_assert("first stmt should be goto", n1->kind == ND_GOTO);
    mu_assert("goto label token should exist", n1->tok != NULL);
    mu_assert("goto label should be L",
              n1->tok->len == 1 && n1->tok->str[0] == 'L');

    Node* n2 = parse_stmt(&ctx);
    mu_assert("second stmt should be label", n2->kind == ND_LABEL);
    mu_assert("label token should exist", n2->tok != NULL);
    mu_assert("label stmt should wrap return",
              n2->lhs && n2->lhs->kind == ND_RETURN);

    free_ast(n1);
    free_ast(n2);
    free_tokens(tok);
    return NULL;
}

char* test_parse_designated_initializer_array() {
    Context ctx = {0};
    Token* tok = tokenize("int a[3] = { [2] = 3, [0] = 1 };");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    mu_assert("node should be ND_DECL", node->kind == ND_DECL);
    mu_assert("initializer should be ND_INIT",
              node->init && node->init->kind == ND_INIT);
    mu_assert("init[0] should be 1", node->init->lhs &&
                                         node->init->lhs->kind == ND_NUM &&
                                         node->init->lhs->val == 1);
    mu_assert("init[1] should be 0",
              node->init->lhs->next && node->init->lhs->next->kind == ND_NUM &&
                  node->init->lhs->next->val == 0);
    mu_assert("init[2] should be 3",
              node->init->lhs->next->next &&
                  node->init->lhs->next->next->kind == ND_NUM &&
                  node->init->lhs->next->next->val == 3);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_designated_initializer_struct() {
    Context ctx = {0};
    Token* tok = tokenize("struct { int a; int b; } s = { .b = 5, .a = 2 };");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    mu_assert("node should be ND_DECL", node->kind == ND_DECL);
    mu_assert("initializer should be ND_INIT",
              node->init && node->init->kind == ND_INIT);
    mu_assert("init.a should be 2", node->init->lhs &&
                                        node->init->lhs->kind == ND_NUM &&
                                        node->init->lhs->val == 2);
    mu_assert("init.b should be 5", node->init->lhs->next &&
                                        node->init->lhs->next->kind == ND_NUM &&
                                        node->init->lhs->next->val == 5);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_long_double_decl() {
    Context ctx = {0};
    Token* tok = tokenize("long double x = 1.5;");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    mu_assert("node should be ND_DECL", node->kind == ND_DECL);
    mu_assert("type should map to DOUBLE",
              node->type && node->type->ty == DOUBLE);
    mu_assert("initializer should be float literal",
              node->init && node->init->kind == ND_FNUM);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_scalar_compound_literal() {
    Context ctx = {0};
    Token* tok = tokenize("int x = (int){3};");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    mu_assert("node should be ND_DECL", node->kind == ND_DECL);
    mu_assert("initializer should exist", node->init != NULL);
    mu_assert("initializer should be cast node", node->init->kind == ND_CAST);
    mu_assert("cast operand should be number",
              node->init->lhs && node->init->lhs->kind == ND_NUM &&
                  node->init->lhs->val == 3);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_complex_decl() {
    Context ctx = {0};
    Token* tok = tokenize("double _Complex x = 1.25;");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    mu_assert("node should be ND_DECL", node->kind == ND_DECL);
    mu_assert("type should map to DOUBLE",
              node->type && node->type->ty == DOUBLE);
    mu_assert("initializer should be float literal",
              node->init && node->init->kind == ND_FNUM);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}

char* test_parse_vla_decl() {
    Context ctx = {0};
    Token* tok = tokenize("int n = 3; int a[n];");
    ctx.current_token = tok;

    Node* n_decl = parse_stmt(&ctx);
    Node* a_decl = parse_stmt(&ctx);

    mu_assert("n decl should be ND_DECL", n_decl->kind == ND_DECL);
    mu_assert("a decl should be ND_DECL", a_decl->kind == ND_DECL);
    mu_assert("a should be marked VLA", a_decl->is_vla);
    mu_assert("a VLA size expression should exist", a_decl->rhs != NULL);
    mu_assert("a type should be pointer for runtime VLA",
              a_decl->type && a_decl->type->ty == PTR);

    free_ast(n_decl);
    free_ast(a_decl);
    free_tokens(tok);
    return NULL;
}

char* test_parse_adjacent_string_literals() {
    Context ctx = {0};
    Token* tok = tokenize("char* s = \"foo\" \"bar\";");
    ctx.current_token = tok;
    Node* node = parse_stmt(&ctx);

    mu_assert("node should be ND_DECL", node->kind == ND_DECL);
    mu_assert("initializer should be ND_STR",
              node->init && node->init->kind == ND_STR);
    mu_assert("one string should be recorded", ctx.string_count == 1);
    mu_assert("concatenated length should be 6", ctx.string_lens[0] == 6);
    mu_assert("concatenated content should be foobar",
              strncmp(ctx.strings[0], "foobar", 6) == 0);

    free_ast(node);
    free_tokens(tok);
    return NULL;
}
