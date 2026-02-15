#include <stdio.h>
#include <stdlib.h>

#include "file.h"
#include "generate.h"

// External declarations

// Make AST from source string
int main() {
    const char* filename = "demo/example01.c";
    const char* source = read_file(filename);
    if (source == NULL) {
        printf("Error: could not read file %s\n", filename);
        return 1;
    }

    printf("Compiling: %s\n\n", source);

    // TODO: lexer and parser not available yet

    if (source != NULL) {
        free((void*)source);
        source = NULL;
    }

    // Make an AST manually
    ReturnExpr* my_ast = malloc(sizeof(ReturnExpr));
    my_ast->value = 42;

    // generate IR from the AST
    generate_code(my_ast);

    free(my_ast);
    return 0;
}
