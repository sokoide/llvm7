#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "sokoide_module"

// 1. Simple AST
typedef struct {
    int value;
} ReturnExpr;

// 2. generate LLVM IR from AST
void generate_code(ReturnExpr* ast) {
    LLVMModuleRef module = LLVMModuleCreateWithName(MODULE_NAME);
    LLVMBuilderRef builder = LLVMCreateBuilder();

    // main function
    LLVMTypeRef ret_type = LLVMInt32Type();
    LLVMTypeRef func_type = LLVMFunctionType(ret_type, NULL, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", func_type);

    // basic block
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    // load value of the AST, issue `Ret` instruction
    LLVMValueRef res = LLVMConstInt(LLVMInt32Type(), ast->value, 0);
    LLVMBuildRet(builder, res);

    // Output IR
    LLVMDumpModule(module);

    // Clean up
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
}

// make AST from source string
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