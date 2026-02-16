#ifndef __PARSE_H__
#define __PARSE_H__

#include "common.h"

// program = function*
// function = ident "(" ")" "{" stmt* "}"
// stmt = expr ";"
//      | "{" stmt* "}"
//      | "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" stmt expr ";" expr ")" stmt
// expr = assign
// assign = equality("=" assign)?
// equality = relational("==" relational | "!=" relational)*
// relational = add("<" add | "<=" add | ">" add | ">=" add)*
// add = mul("+" mul | "-" mul)*
// mul = unary("*" unary | "/" unary)*
// unary = ("+" | "-")? primary
// primary = num
//         | ident ("(" args? ")")?
//         | "(" expr ")"
// args = expr ("," expr)*

extern Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
extern Node* new_node_num(int val);
extern Node* new_node_ident(Context* ctx, Token* tok);
extern void free_ast(Node* ast);
extern void program(Context* ctx);
extern Node* stmt(Context* ctx);
extern Node* expr(Context* ctx);
extern Node* assign(Context* ctx);
extern Node* equality(Context* ctx);
extern Node* relational(Context* ctx);
extern Node* add(Context* ctx);
extern Node* mul(Context* ctx);
extern Node* unary(Context* ctx);
extern Node* primary(Context* ctx);
extern Node* args(Context* ctx);
extern Node* function(Context* ctx);

#endif
