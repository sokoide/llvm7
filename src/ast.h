#ifndef __AST_H__
#define __AST_H__

// expr =       equality
// equality =   relational("==" relational | "!=" relational)*
// relational = add("<" add | "<=" add | ">" add | ">=" add)*
// add =        mul ("+" mul | "-" mul)*
// mul =        unary("*" unary | "/" unary)*
// unary =      ("+" | "-") ? primary
// primary =    num | "(" expr ")"

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // Number
    ND_LT,  // <
    ND_LE,  // <=
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_GE,  // >=
    ND_GT,  // >
} NodeKind;

struct Node {
    NodeKind kind;
    struct Node* lhs;
    struct Node* rhs;
    int val;
};

typedef struct Node Node;

extern Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
extern Node* new_node_num(int val);
extern void free_ast(Node* ast);
extern Node* expr();
extern Node* equality();
extern Node* relational();
extern Node* add();
extern Node* mul();
extern Node* unary();
extern Node* primary();

#endif