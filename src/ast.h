#ifndef __AST_H__
#define __AST_H__

// program = stmt*
// stmt = expr ";"
// expr = assign
// assign = equality("=" assign) ?
// equality =   relational("==" relational | "!="
// relational)* relational = add("<" add | "<=" add |
// ">" add | ">=" add)* add =        mul ("+" mul | "-"
// mul)* mul =        unary("*" unary | "/" unary)*
// unary =      ("+" | "-") ? primary
// primary =    num | ident | "(" expr ")"

typedef enum {
    ND_ADD,    // +
    ND_SUB,    // -
    ND_MUL,    // *
    ND_DIV,    // /
    ND_LT,     // <
    ND_LE,     // <=
    ND_EQ,     // ==
    ND_NE,     // !=
    ND_GE,     // >=
    ND_GT,     // >
    ND_ASSIGN, // = (assignment)
    ND_LVAR,   // local var
    ND_NUM,    // Integer
} NodeKind;

struct Node {
    NodeKind kind;
    struct Node* lhs;
    struct Node* rhs;
    int val;    // used if kind = ND_NUM
    int offset; // used if kind = ND_LVAR
};

typedef struct Node Node;

extern Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
extern Node* new_node_num(int val);
extern Node* new_node_ident(const char* name);
extern void free_ast(Node* ast);
extern void program();
extern Node* stmt();
extern Node* expr();
extern Node* assign();
extern Node* equality();
extern Node* relational();
extern Node* add();
extern Node* mul();
extern Node* unary();
extern Node* primary();

#define MAX_NODES 128
extern Node* code[MAX_NODES];

#endif