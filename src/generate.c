#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "generate.h"

#define MODULE_NAME "sokoide_module"

static LLVMValueRef codegen(Node* node, LLVMBuilderRef builder);

/**
 * Internal function: Generates LLVM IR module from a Node AST
 *
 * @param[in] ast Pointer to Node AST
 * @return LLVMModuleRef Reference to generated LLVM module
 */
LLVMModuleRef generate_module(Node* ast) {
    // Create a new LLVM module with specified name
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

    // Generate code from AST
    LLVMValueRef res = codegen(ast, builder);
    LLVMBuildRet(builder, res);

    // Clean up builder
    LLVMDisposeBuilder(builder);

    return module;
}

/**
 * Recursively generates LLVM IR code from AST nodes
 *
 * @param[in] node Pointer to Node AST
 * @param[in] builder LLVM builder
 * @return LLVMValueRef Generated value
 */
static LLVMValueRef codegen(Node* node, LLVMBuilderRef builder) {
    if (node == NULL) {
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }

    switch (node->kind) {
    case ND_NUM: {
        return LLVMConstInt(LLVMInt32Type(), node->val, 0);
    }
    case ND_ADD: {
        LLVMValueRef lhs = codegen(node->lhs, builder);
        LLVMValueRef rhs = codegen(node->rhs, builder);
        return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
    }
    case ND_SUB: {
        LLVMValueRef lhs = codegen(node->lhs, builder);
        LLVMValueRef rhs = codegen(node->rhs, builder);
        return LLVMBuildSub(builder, lhs, rhs, "subtmp");
    }
    case ND_MUL: {
        LLVMValueRef lhs = codegen(node->lhs, builder);
        LLVMValueRef rhs = codegen(node->rhs, builder);
        return LLVMBuildMul(builder, lhs, rhs, "multmp");
    }
    case ND_DIV: {
        LLVMValueRef lhs = codegen(node->lhs, builder);
        LLVMValueRef rhs = codegen(node->rhs, builder);
        return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
    }
    default:
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
}

/**
 * Generates LLVM IR code in stdout from a Node AST
 *
 * @param[in] ast Pointer to Node AST
 */
void generate_code(Node* ast) {
    LLVMModuleRef module = generate_module(ast);

    // Output IR
    LLVMDumpModule(module);

    // Clean up module
    LLVMDisposeModule(module);
}
