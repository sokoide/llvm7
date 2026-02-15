#include "generate.h"
#include "ast.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "sokoide_module"

static LLVMValueRef codegen(Node* node, LLVMBuilderRef builder, LLVMValueRef local_vars);

/**
 * Internal function: Generates LLVM IR module from a Node AST
 *
 * @param[in] ast Pointer to Node AST
 * @return LLVMModuleRef Reference to generated LLVM module
 */
LLVMModuleRef generate_module(void) {
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

    // local variable space per function (temporary)
    LLVMTypeRef i64_type = LLVMInt64Type();
    LLVMTypeRef array_type = LLVMArrayType(i64_type, 26);
    LLVMValueRef local_vars =
        LLVMBuildAlloca(builder, array_type, "local_vars");

    // Generate code from AST statements
    LLVMValueRef res = LLVMConstInt(LLVMInt32Type(), 0, 0);
    for (int i = 0; code[i] != NULL; i++) {
        res = codegen(code[i], builder, local_vars);
    }
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
static LLVMValueRef codegen(Node* node, LLVMBuilderRef builder, LLVMValueRef local_vars) {
    if (node == NULL) {
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }

    switch (node->kind) {
    case ND_NUM: {
        return LLVMConstInt(LLVMInt32Type(), node->val, 0);
    }
    case ND_LVAR: {
        LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), node->val, false);
        LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, false),
                                  index};
        LLVMValueRef gep =
            LLVMBuildInBoundsGEP2(builder, LLVMInt64Type(), local_vars, indices, 2, "arrayidx");

        return LLVMBuildLoad2(builder, LLVMInt32Type(), gep, "loadtmp");
    }
    case ND_ASSIGN: {
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars);
        LLVMValueRef index =
            LLVMConstInt(LLVMInt32Type(), node->lhs->val, false);

        LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, false),
                                  index};
        LLVMValueRef gep =
            LLVMBuildInBoundsGEP2(builder, LLVMInt64Type(), local_vars, indices, 2, "arrayidx");

        LLVMBuildStore(builder, rhs, gep);
        return rhs;  // assignment returns the assigned value
    }
    case ND_ADD: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars);
        return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
    }
    case ND_SUB: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars);
        return LLVMBuildSub(builder, lhs, rhs, "subtmp");
    }
    case ND_MUL: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars);
        return LLVMBuildMul(builder, lhs, rhs, "multmp");
    }
    case ND_DIV: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars);
        return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
    }
    default:
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
}

/**
 * Generates LLVM IR code in stdout from code[] array
 */
void generate_code(void) {
    LLVMModuleRef module = generate_module();

    // Output IR
    LLVMDumpModule(module);

    // Clean up module
    LLVMDisposeModule(module);
}
