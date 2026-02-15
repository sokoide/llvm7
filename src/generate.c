#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generate.h"

#define MODULE_NAME "sokoide_module"

void generate_code(ReturnExpr* ast);
LLVMModuleRef generate_module(ReturnExpr* ast);

/**
 * Internal function: Generates LLVM IR module from a ReturnExpr AST node
 *
 * @param[in] ast Pointer to the ReturnExpr AST node containing the return
 * value
 * @return LLVMModuleRef Reference to the generated LLVM module
 */
LLVMModuleRef generate_module(ReturnExpr* ast) {
    // Create a new LLVM module with the specified name
    LLVMModuleRef module = LLVMModuleCreateWithName(MODULE_NAME);
    // Create an LLVM builder for constructing instructions
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

    // Clean up builder
    LLVMDisposeBuilder(builder);

    return module;
}

/**
 * Generates LLVM IR code in stdout from a ReturnExpr AST node
 *
 * @param[in] ast Pointer to the ReturnExpr AST node containing the return
 * value
 */
void generate_code(ReturnExpr* ast) {
    LLVMModuleRef module = generate_module(ast);

    // Output IR
    LLVMDumpModule(module);

    // Clean up module
    LLVMDisposeModule(module);
}