#include <stdio.h>
#include <stdlib.h>

// External declarations
typedef struct {
    int value;
} ReturnExpr;

extern void generate_code(ReturnExpr* ast);

// Make AST from source string
int main() {
    const char* source = "int main() { return 42; }";
    printf("Compiling: %s\n\n", source);

    // TODO: lexer and parser not available yet
    // Make an AST manually
    ReturnExpr* my_ast = malloc(sizeof(ReturnExpr));
    my_ast->value = 42;

    // generate IR from the AST
    generate_code(my_ast);

    free(my_ast);
    return 0;
}
