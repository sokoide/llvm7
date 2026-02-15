#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "file.h"
#include "generate.h"
#include "lexer.h"

int main(int argc, const char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    const char* filename = argv[1];
    const char* source = read_file(filename);
    if (source == NULL) {
        printf("Error: could not read file %s\n", filename);
        return 1;
    }

    printf("Compiling: %s\n\n", source);

    // Tokenize
    token = NULL;
    token = tokenize(source);

    // Parse AST
    program();

    // Generate LLVM IR
    generate_code();

    // Clean up
    free((void*)source);

    return 0;
}
