#ifndef __PARSE_H__
#define __PARSE_H__

#include "common.h"

// program = function*
// function = type ident "(" params? ")" "{" stmt* "}"
// params = type ident ("," type ident)*
// type = "int" | "void" | type "*"
// stmt = expr ";"
//      | "{" stmt* "}"
//      | "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" stmt expr ";" expr ")" stmt
//      | type ident ";"
// expr = assign
// assign = equality("=" assign)?
// equality = relational("==" relational | "!=" relational)*
// relational = add("<" add | "<=" add | ">" add | ">=" add)*
// add = mul("+" mul | "-" mul)*
// mul = unary("*" unary | "/" unary)*
// unary = ("+" | "-")? primary
//       | "*" unary
//       | "&" unary
// primary = num
//         | ident ("(" args? ")")?
//         | "(" expr ")"
// args = expr ("," expr)*

extern Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
extern Node* new_node_num(int val);
extern Node* new_node_ident(Context* ctx, Token* tok);
extern void free_ast(Node* ast);
extern void parse_program(Context* ctx);
extern Node* parse_stmt(Context* ctx);
extern Node* parse_expr(Context* ctx);
extern Node* parse_assign(Context* ctx);
extern Node* parse_equality(Context* ctx);
extern Node* parse_relational(Context* ctx);
extern Node* parse_add(Context* ctx);
extern Node* parse_mul(Context* ctx);
extern Node* parse_unary(Context* ctx);
extern Node* parse_primary(Context* ctx);
extern Node* parse_args(Context* ctx);
extern Node* parse_function(Context* ctx);
extern Node* parse_params(Context* ctx);
extern Type* parse_type(Context* ctx);
extern Type* try_parse_type(Context* ctx);
extern Type* new_type_int(void);
extern Type* new_type_ptr(Type* base);

#endif
