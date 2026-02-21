#ifndef __PARSE_H__
#define __PARSE_H__

#include "common.h"

// program      = (extern? (typedef | function | global_decl))*
// typedef      = "typedef" type ident ";"
// function     = type ident "(" params? ")" ( "{" stmt* "}" | ";" )
// params       = param ("," param)* ("," "...")?
// param        = type ident? ("[" num? "]")?
// type         = qualifier* base_type ("*" | "(" type ")" | "[" num? "]")*
// qualifier    = "const" | "static" | "extern" | "signed" | "unsigned"
// base_type    = "int" | "char" | "void" | "long" ("long")? | "bool"
//              | "size_t" | struct_decl | enum_decl | typedef_name
// struct_decl  = "struct" ident? ("{" member* "}")?
// member       = type ident ("[" num? "]")? ";"
// enum_decl    = "enum" ident? ("{" enum_entry ("," enum_entry)* ","? "}")?
// enum_entry   = ident ("=" expr)?
// global_decl  = type ident ("[" expr? "]")? ("=" (expr | initializer))? ";"
// initializer  = "{" expr ("," expr)* ","? "}"
// stmt         = declaration
//              | expr? ";"
//              | "{" stmt* "}"
//              | "return" expr? ";"
//              | "if" "(" expr ")" stmt ("else" stmt)?
//              | "while" "(" expr ")" stmt
//              | "for" "(" (declaration | expr?) ";" expr? ";" expr? ")" stmt
//              | "switch" "(" expr ")" stmt
//              | "case" expr ":"
//              | "default" ":"
//              | "break" ";"
//              | "continue" ";"
// declaration  = type ident ("[" expr? "]")? ("=" (expr | initializer))? ";"
// expr         = assign
// assign       = conditional (("=" | "+=" | "-=" | "*=" | "/=") assign)?
// conditional  = logor ("?" expr ":" conditional)?
// logor        = logand ("||" logand)*
// logand       = equality ("&&" equality)*
// equality     = relational ("==" relational | "!=" relational)*
// relational   = add ("<" add | "<=" add | ">" add | ">=" add)*
// add          = mul ("+" mul | "-" mul)*
// mul          = unary ("*" unary | "/" unary | "%" unary)*
// unary        = "sizeof" (unary | "(" type ")")
//              | ("+" | "-" | "*" | "&" | "!" | "++" | "--") unary
//              | "(" type ")" unary
//              | postfix
// postfix      = primary ("[" expr "]" | "." ident | "->" ident | "++" | "--")*
// primary      = num | "NULL" | "true" | "false" | ident ("(" args? ")")?
//              | string | "(" expr ")"
// args         = expr ("," expr)*

extern Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
extern Node* new_node_num(int val);
extern Node* new_node_ident(Context* ctx, Token* tok);
extern void free_ast(Node* ast);
extern void parse_program(Context* ctx);
extern Node* parse_stmt(Context* ctx);
Node* parse_declaration(Context* ctx, Type* ty);
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
extern Node* parse_params(Context* ctx, bool* is_vararg);
extern Type* parse_type(Context* ctx);
extern Type* try_parse_type(Context* ctx);
extern Type* new_type_int(void);
extern Type* new_type_char(void);
extern Type* new_type_ptr(Type* base);

#endif
