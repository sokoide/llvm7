#include "generate.h"
#include "ast.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "sokoide_module"

static LLVMValueRef codegen(Node* node, LLVMBuilderRef builder,
                            LLVMValueRef local_vars, LLVMTypeRef array_type);

/**
 * Internal function: Generates LLVM IR module from a Node AST
 *
 * @param[in] ctx Context containing AST nodes
 * @return LLVMModuleRef Reference to generated LLVM module
 */
LLVMModuleRef generate_module(Context* ctx) {
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
    LLVMTypeRef i32_type = LLVMInt32Type();
    LLVMTypeRef array_type = LLVMArrayType(i32_type, 26);
    LLVMValueRef local_vars =
        LLVMBuildAlloca(builder, array_type, "local_vars");

    // Generate code from AST statements
    LLVMValueRef res = LLVMConstInt(LLVMInt32Type(), 0, 0);
    for (int i = 0; i < ctx->node_count; i++) {
        res = codegen(ctx->code[i], builder, local_vars, array_type);
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
static LLVMValueRef codegen(Node* node, LLVMBuilderRef builder,
                            LLVMValueRef local_vars, LLVMTypeRef array_type) {
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
        LLVMValueRef gep = LLVMBuildInBoundsGEP2(
            builder, array_type, local_vars, indices, 2, "arrayidx");

        return LLVMBuildLoad2(builder, LLVMInt32Type(), gep, "loadtmp");
    }
    case ND_ASSIGN: {
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        LLVMValueRef index =
            LLVMConstInt(LLVMInt32Type(), node->lhs->val, false);

        LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, false),
                                  index};
        LLVMValueRef gep = LLVMBuildInBoundsGEP2(
            builder, array_type, local_vars, indices, 2, "arrayidx");

        LLVMBuildStore(builder, rhs, gep);
        return rhs; // assignment returns the assigned value
    }
    case ND_ADD: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
    }
    case ND_SUB: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        return LLVMBuildSub(builder, lhs, rhs, "subtmp");
    }
    case ND_MUL: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        return LLVMBuildMul(builder, lhs, rhs, "multmp");
    }
    case ND_DIV: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
    }
    case ND_LT: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "lttmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_LE: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "letmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_EQ: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        LLVMValueRef res = LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eqtmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_NE: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        LLVMValueRef res = LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "netmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_GE: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "getmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_GT: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "gttmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    default:
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
}

/**
 * Generates LLVM IR code in stdout from Context
 *
 * @param[in] ctx Context containing AST nodes
 */
void generate_code(Context* ctx) {
    LLVMModuleRef module = generate_module(ctx);

    // Output IR
    LLVMDumpModule(module);

    // Clean up module
    LLVMDisposeModule(module);
}

/**
 * Generates LLVM IR code to a file
 *
 * @param[in] ctx Context containing AST nodes
 * @param[in] filename Output filename
 * @return 0 on success, non-zero on failure
 */
int generate_code_to_file(Context* ctx, const char* filename) {
    LLVMModuleRef module = generate_module(ctx);

    // Write IR to file
    char* error = NULL;
    int result = LLVMPrintModuleToFile(module, filename, &error);

    if (error) {
        fprintf(stderr, "Error writing to file: %s\n", error);
        LLVMDisposeMessage(error);
    }

    // Clean up module
    LLVMDisposeModule(module);

    return result;
}
