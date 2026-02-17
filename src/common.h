#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <stddef.h>

#define MAX_NODES 128

typedef enum {
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
    TK_STR,
    TK_EOF,
} TokenKind;

typedef struct Type Type;
typedef struct Member Member;
struct Member {
    Member* next;
    Type* type;
    const char* name;
    int len;
    int index; // Member index for GEP
};

typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token* next;
    int val;
    const char* str;
    int len;
};

struct Type {
    enum { INT, CHAR, VOID, PTR, STRUCT } ty;
    Type* ptr_to; // For PTR, points to the type being pointed to
    size_t array_size;
    Member* members; // For STRUCT
};

typedef enum {
    ND_ADD,          // +
    ND_SUB,          // -
    ND_MUL,          // *
    ND_DIV,          // /
    ND_LT,           // <
    ND_LE,           // <=
    ND_EQ,           // ==
    ND_NE,           // !=
    ND_GE,           // >=
    ND_GT,           // >
    ND_ASSIGN,       // = (assignment)
    ND_LVAR,         // local var
    ND_GVAR,         // global var
    ND_NUM,          // Integer
    ND_RETURN,       // return
    ND_IF,           // if
    ND_WHILE,        // while
    ND_FOR,          // for
    ND_BLOCK,        // block
    ND_CALL,         // call
    ND_FUNCTION,     // function definition
    ND_DEREF,        // * (pointer dereference)
    ND_ADDR,         // & (address-of)
    ND_DECL,         // local variable declaration
    ND_STR,          // string literal
    ND_MEMBER,       // struct member access (.)
    ND_ARRAY_TO_PTR, // Implicit array to pointer conversion
    ND_LOGAND,       // &&
    ND_LOGOR,        // ||
    ND_NOT,          // !
    ND_SWITCH,       // switch
    ND_CASE,         // case
    ND_BREAK,        // break
    ND_PRE_INC,      // ++i
    ND_POST_INC,     // i++
    ND_PRE_DEC,      // --i
    ND_POST_DEC,     // i--
} NodeKind;

typedef struct LVar LVar;
struct LVar {
    LVar* next;
    const char* name;
    int len;
    int offset;
    Type* type; // Type of the variable
};

typedef struct Typedef Typedef;
struct Typedef {
    Typedef* next;
    const char* name;
    int len;
    Type* type;
};

typedef struct EnumConst EnumConst;
struct EnumConst {
    EnumConst* next;
    const char* name;
    int len;
    int val;
};

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
    Type* type;       // Type of the node (for ND_DECL, ND_LVAR, etc.)
    LVar* locals;     // Local variables (for ND_FUNCTION)
    Member* member;   // for ND_MEMBER
    Node* cases;      // for ND_SWITCH
    Node* next_case;  // for case/default list in ND_SWITCH
    int case_val;     // for ND_CASE
    bool is_default;  // for ND_DEFAULT
    void* llvm_label; // LLVMBasicBlockRef for ND_CASE/ND_BREAK/etc.
};

typedef struct Context Context;
struct Context {
    Token* current_token;           // Current token being processed
    Node* code[MAX_NODES];          // Generated AST nodes (statements)
    LVar* locals;                   // local variables
    LVar* globals;                  // global variables
    Typedef* typedefs;              // typedefs
    EnumConst* enum_consts;         // enum constants
    Node* current_switch;           // Nested switch tracking
    void* current_switch_inst;      // Switch instruction for current context
    void* current_break_label;      // Current jump target for break
    int node_count;                 // Number of statements
    const char* strings[MAX_NODES]; // string literal data
    int string_lens[MAX_NODES];     // string literal lengths
    int string_count;               // number of string literals
};

#endif
