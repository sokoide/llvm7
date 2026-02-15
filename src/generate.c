#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generate.h"

#define MODULE_NAME "sokoide_module"

void generate_code(ReturnExpr* ast);
LLVMModuleRef generate_code_to_module(ReturnExpr* ast);

// generate LLVM IR from AST (dumps to stdout)
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

// generate LLVM IR from AST and return module (for testing)
LLVMModuleRef generate_code_to_module(ReturnExpr* ast) {
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

    // Clean up builder but keep module for external use
    LLVMDisposeBuilder(builder);

    return module;
}
