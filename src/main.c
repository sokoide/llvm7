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

    // Create context and tokenize
    Context ctx = {0};
    ctx.current_token = tokenize(source);

    // Parse AST
    program(&ctx);

    // Generate LLVM IR
    generate_code(&ctx);

    // Clean up
    free((void*)source);
    for (int i = 0; i < ctx.node_count; i++) {
        free_ast(ctx.code[i]);
    }
    if (ctx.current_token) {
        free_tokens(ctx.current_token);
    }

    return 0;
}
