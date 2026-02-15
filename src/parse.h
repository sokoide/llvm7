#ifndef __PARSE_H__
#define __PARSE_H__

#include "common.h"

extern Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
extern Node* new_node_num(int val);
extern Node* new_node_ident(const char* name);
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

#endif
