#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "file.h"
#include "lex.h"
#include "parse.h"

int main(int argc, const char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file> [-o <output_file>]\n", argv[0]);
        fprintf(stderr, "  Default output: tmp.ll\n");
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = "tmp.ll"; // default

    // Parse -o option
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[i + 1];
                i++;
            } else {
                fprintf(stderr, "Error: -o requires a filename argument\n");
                return 1;
            }
        }
    }

    const char* source = read_file(input_file);
    if (source == NULL) {
        printf("Error: could not read file %s\n", input_file);
        return 1;
    }

    printf("Compiling: %s\n", source);
    printf("Output: %s\n\n", output_file);

    // Create context and tokenize
    Context ctx = {0};
    ctx.current_token = tokenize(source);

    // Parse AST
    parse_program(&ctx);

    // Generate LLVM IR to file
    if (generate_code_to_file(&ctx, output_file) != 0) {
        fprintf(stderr, "Error: failed to generate LLVM IR\n");
        free((void*)source);
        if (ctx.current_token) {
            free_tokens(ctx.current_token);
        }
        return 1;
    }

    printf("Generated: %s\n", output_file);

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
