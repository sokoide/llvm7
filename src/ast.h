#ifndef __AST_H__
#define __AST_H__

#include <llvm-c/Core.h>

#define MAX_NODES 128

// Forward declarations
struct Token;
struct Node;

// Context structure to hold parser and lexer state
struct Context {
    struct Token* current_token;  // Current token being processed
    struct Node* code[MAX_NODES]; // Generated AST nodes (statements)
    int node_count;               // Number of statements
};

// Typedefs - guarded to avoid redefinition warnings
#ifndef CONTEXT_TYPEDEF_DEFINED
#define CONTEXT_TYPEDEF_DEFINED
typedef struct Context Context;
#endif

#ifndef TOKEN_TYPEDEF_DEFINED
#define TOKEN_TYPEDEF_DEFINED
typedef struct Token Token;
#endif

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
    int val;
};

#ifndef NODE_TYPEDEF_DEFINED
#define NODE_TYPEDEF_DEFINED
typedef struct Node Node;
#endif

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