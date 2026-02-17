#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>

#define MAX_NODES 128

typedef enum {
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token* next;
    int val;
    const char* str;
    int len;
};

typedef enum {
    TY_INT,
    TY_VOID,
} Type;

typedef enum {
    ND_ADD,      // +
    ND_SUB,      // -
    ND_MUL,      // *
    ND_DIV,      // /
    ND_LT,       // <
    ND_LE,       // <=
    ND_EQ,       // ==
    ND_NE,       // !=
    ND_GE,       // >=
    ND_GT,       // >
    ND_ASSIGN,   // = (assignment)
    ND_LVAR,     // local var
    ND_NUM,      // Integer
    ND_RETURN,   // return
    ND_IF,       // if
    ND_WHILE,    // while
    ND_FOR,      // for
    ND_BLOCK,    // block
    ND_CALL,     // call
    ND_FUNCTION, // function definition
    ND_DEREF,    // * (pointer dereference)
    ND_ADDR,     // & (address-of)
    ND_DECL,     // local variable declaration
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node* next;
    Node* lhs;  // General left operand, or then branch for if/while/for
    Node* rhs;  // General right operand, or else branch for if, or inc for for
    Node* cond; // Condition for if/while/for
    Node* init; // Initialization for for loop
    Token* tok; // Function name or token for the node
    int val;
};

typedef struct LVar LVar;

struct LVar {
    LVar* next;
    const char* name;
    int len;
    int offset;
};

typedef struct Context Context;
struct Context {
    Token* current_token;  // Current token being processed
    Node* code[MAX_NODES]; // Generated AST nodes (statements)
    LVar* locals;          // local variables
    int node_count;        // Number of statements
};

#endif
