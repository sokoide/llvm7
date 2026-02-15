#ifndef __AST_H__
#define __AST_H__

// expr    = mul ("+" mul | "-" mul)*
// mul = unary("*" unary | "/" unary)*
// unary = ("+" | "-") ? primary
// primary = num | "(" expr ")"

typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
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
extern Node* mul();
extern Node* unary();
extern Node* primary();

#endif