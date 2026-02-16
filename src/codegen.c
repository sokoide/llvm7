#include "codegen.h"
#include "parse.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "sokoide_module"

static LLVMValueRef codegen(Node* node, LLVMBuilderRef builder,
                            LLVMValueRef local_vars, LLVMTypeRef array_type,
                            bool* has_return, LLVMModuleRef module);

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

    // Track if main function exists
    bool has_main = false;

    // Generate code for each function
    for (int i = 0; i < ctx->node_count; i++) {
        Node* func_node = ctx->code[i];

        if (func_node->kind != ND_FUNCTION) {
            fprintf(stderr, "Expected function definition\n");
            continue;
        }

        // Get function name
        char func_name[64];
        int len = func_node->tok->len < 63 ? func_node->tok->len : 63;
        strncpy(func_name, func_node->tok->str, len);
        func_name[len] = '\0';

        // Check for main function
        if (strcmp(func_name, "main") == 0) {
            has_main = true;
        }

        // Count parameters
        int param_count = 0;
        Node* param = func_node->rhs;
        while (param != NULL) {
            param_count++;
            param = param->next;
        }

        // Create function type with parameters
        LLVMTypeRef ret_type = LLVMInt32Type();
        LLVMTypeRef* param_types = NULL;
        if (param_count > 0) {
            param_types = malloc(param_count * sizeof(LLVMTypeRef));
            for (int i = 0; i < param_count; i++) {
                param_types[i] = LLVMInt32Type();
            }
        }
        LLVMTypeRef func_type =
            LLVMFunctionType(ret_type, param_types, param_count, 0);
        LLVMValueRef func = LLVMAddFunction(module, func_name, func_type);

        // Create entry block
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
        LLVMPositionBuilderAtEnd(builder, entry);

        // Local variable space per function (temporary)
        LLVMTypeRef i32_type = LLVMInt32Type();
        LLVMTypeRef array_type = LLVMArrayType(i32_type, 26);
        LLVMValueRef local_vars =
            LLVMBuildAlloca(builder, array_type, "local_vars");

        // Store parameter values into local variables
        param = func_node->rhs;
        for (int i = 0; i < param_count; i++) {
            LLVMValueRef arg = LLVMGetParam(func, i);
            LLVMValueRef index =
                LLVMConstInt(LLVMInt32Type(), param->val, false);
            LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, false),
                                      index};
            LLVMValueRef gep = LLVMBuildInBoundsGEP2(
                builder, array_type, local_vars, indices, 2, "arrayidx");
            LLVMBuildStore(builder, arg, gep);
            param = param->next;
        }

        if (param_types) {
            free(param_types);
        }

        // Generate function body statements
        LLVMValueRef res = LLVMConstInt(LLVMInt32Type(), 0, 0);
        bool has_return = false;
        Node* stmt = func_node->lhs;
        while (stmt != NULL) {
            res = codegen(stmt, builder, local_vars, array_type, &has_return,
                          module);
            if (has_return) {
                break;
            }
            stmt = stmt->next;
        }
        if (!has_return) {
            LLVMBuildRet(builder, res);
        }
    }

    if (!has_main) {
        fprintf(stderr, "Error: main function is required\n");
        LLVMDisposeBuilder(builder);
        LLVMDisposeModule(module);
        exit(1);
    }

    // Clean up builder
    LLVMDisposeBuilder(builder);

    return module;
}

/**
 * Recursively generates LLVM IR code from AST nodes
 *
 * @param[in] node Pointer to Node AST
 * @param[in] builder LLVM builder
 * @param[out] has_return Set to true if return statement was encountered
 * @return LLVMValueRef Generated value
 */
static LLVMValueRef codegen(Node* node, LLVMBuilderRef builder,
                            LLVMValueRef local_vars, LLVMTypeRef array_type,
                            bool* has_return, LLVMModuleRef module) {
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
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
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
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
    }
    case ND_SUB: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        return LLVMBuildSub(builder, lhs, rhs, "subtmp");
    }
    case ND_MUL: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        return LLVMBuildMul(builder, lhs, rhs, "multmp");
    }
    case ND_DIV: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
    }
    case ND_LT: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "lttmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_LE: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "letmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_EQ: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef res = LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eqtmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_NE: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef res = LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "netmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_GE: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "getmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_GT: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef rhs = codegen(node->rhs, builder, local_vars, array_type,
                                   has_return, module);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "gttmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_CALL: {
        char func_name[64];
        int len = node->tok->len < 63 ? node->tok->len : 63;
        strncpy(func_name, node->tok->str, len);
        func_name[len] = '\0';

        // Count arguments and generate code for each
        int arg_count = 0;
        Node* arg = node->lhs;
        while (arg != NULL) {
            arg_count++;
            arg = arg->next;
        }

        // Generate argument values
        LLVMValueRef* args = NULL;
        if (arg_count > 0) {
            args = malloc(arg_count * sizeof(LLVMValueRef));
            arg = node->lhs;
            for (int i = 0; i < arg_count; i++) {
                args[i] = codegen(arg, builder, local_vars, array_type,
                                  has_return, module);
                arg = arg->next;
            }
        }

        LLVMValueRef func = LLVMGetNamedFunction(module, func_name);
        LLVMTypeRef func_type;
        if (!func) {
            LLVMTypeRef i32_type = LLVMInt32Type();
            // Create parameter types array
            LLVMTypeRef* param_types = NULL;
            if (arg_count > 0) {
                param_types = malloc(arg_count * sizeof(LLVMTypeRef));
                for (int i = 0; i < arg_count; i++) {
                    param_types[i] = i32_type;
                }
            }
            func_type = LLVMFunctionType(i32_type, param_types, arg_count, 0);
            func = LLVMAddFunction(module, func_name, func_type);
            if (param_types) {
                free(param_types);
            }
        } else {
            func_type = LLVMGlobalGetValueType(func);
        }

        LLVMValueRef result = LLVMBuildCall2(builder, func_type, func, args,
                                             arg_count, "calltmp");
        if (args) {
            free(args);
        }
        return result;
    }
    case ND_RETURN: {
        LLVMValueRef lhs = codegen(node->lhs, builder, local_vars, array_type,
                                   has_return, module);
        *has_return = true;
        return LLVMBuildRet(builder, lhs);
    }
    case ND_IF: {
        // Generate condition
        LLVMValueRef cond_val = codegen(node->cond, builder, local_vars,
                                        array_type, has_return, module);

        // Compare with 0 (condition != 0)
        LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
        LLVMValueRef cond_bool =
            LLVMBuildICmp(builder, LLVMIntNE, cond_val, zero, "ifcond");

        // Create basic blocks
        LLVMBasicBlockRef then_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "then");
        LLVMBasicBlockRef else_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "else");

        // Build conditional branch
        LLVMBuildCondBr(builder, cond_bool, then_bb, else_bb);

        // Generate then block
        LLVMPositionBuilderAtEnd(builder, then_bb);
        bool then_has_return = false;
        codegen(node->lhs, builder, local_vars, array_type, &then_has_return,
                module);
        LLVMBasicBlockRef then_end = LLVMGetInsertBlock(builder);

        // Generate else block
        LLVMPositionBuilderAtEnd(builder, else_bb);
        bool else_has_return = false;
        if (node->rhs) {
            codegen(node->rhs, builder, local_vars, array_type,
                    &else_has_return, module);
        }
        LLVMBasicBlockRef else_end = LLVMGetInsertBlock(builder);

        // If both branches return, no merge block needed
        if (then_has_return && else_has_return) {
            *has_return = true;
            return LLVMConstInt(LLVMInt32Type(), 0, 0);
        }

        // Create merge block
        LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "ifcont");

        // Add branches to merge for non-returning paths
        if (!then_has_return) {
            LLVMPositionBuilderAtEnd(builder, then_end);
            LLVMBuildBr(builder, merge_bb);
        }
        if (!else_has_return) {
            LLVMPositionBuilderAtEnd(builder, else_end);
            LLVMBuildBr(builder, merge_bb);
        }

        // Position at merge block
        LLVMPositionBuilderAtEnd(builder, merge_bb);

        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_WHILE: {
        // Create basic blocks: cond, body, merge
        LLVMBasicBlockRef cond_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "while.cond");
        LLVMBasicBlockRef body_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "while.body");
        LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "while.end");

        // Jump to condition
        LLVMBuildBr(builder, cond_bb);

        // Generate condition block
        LLVMPositionBuilderAtEnd(builder, cond_bb);
        LLVMValueRef cond_val = LLVMConstInt(LLVMInt32Type(), 1, 0);
        if (node->cond) {
            cond_val = codegen(node->cond, builder, local_vars, array_type,
                               has_return, module);
        }
        LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
        LLVMValueRef cond_bool =
            LLVMBuildICmp(builder, LLVMIntNE, cond_val, zero, "while.cond");
        LLVMBuildCondBr(builder, cond_bool, body_bb, merge_bb);

        // Generate body block
        LLVMPositionBuilderAtEnd(builder, body_bb);
        codegen(node->lhs, builder, local_vars, array_type, has_return, module);
        if (!*has_return) {
            LLVMBuildBr(builder, cond_bb); // Loop back
        }

        // Position at merge block
        LLVMPositionBuilderAtEnd(builder, merge_bb);

        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_FOR: {
        // Create basic blocks: cond, body, inc, merge
        LLVMBasicBlockRef cond_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "for.cond");
        LLVMBasicBlockRef body_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "for.body");
        LLVMBasicBlockRef inc_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "for.inc");
        LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "for.end");

        // Generate init if present
        if (node->init) {
            codegen(node->init, builder, local_vars, array_type, has_return,
                    module);
        }

        // Jump to condition
        LLVMBuildBr(builder, cond_bb);

        // Generate condition block
        LLVMPositionBuilderAtEnd(builder, cond_bb);
        LLVMValueRef cond_val = LLVMConstInt(LLVMInt32Type(), 1, 0);
        if (node->cond) {
            cond_val = codegen(node->cond, builder, local_vars, array_type,
                               has_return, module);
        }
        LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
        LLVMValueRef cond_bool =
            LLVMBuildICmp(builder, LLVMIntNE, cond_val, zero, "for.cond");
        LLVMBuildCondBr(builder, cond_bool, body_bb, merge_bb);

        // Generate body block
        LLVMPositionBuilderAtEnd(builder, body_bb);
        codegen(node->lhs, builder, local_vars, array_type, has_return, module);
        if (!*has_return) {
            LLVMBuildBr(builder, inc_bb);
        }

        // Generate increment block
        LLVMPositionBuilderAtEnd(builder, inc_bb);
        if (node->rhs) {
            codegen(node->rhs, builder, local_vars, array_type, has_return,
                    module);
        }
        LLVMBuildBr(builder, cond_bb); // Loop back to condition

        // Position at merge block
        LLVMPositionBuilderAtEnd(builder, merge_bb);

        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_BLOCK: {
        // Execute statements in sequence
        LLVMValueRef result = LLVMConstInt(LLVMInt32Type(), 0, 0);
        Node* stmt = node->lhs;
        while (stmt != NULL) {
            result = codegen(stmt, builder, local_vars, array_type, has_return,
                             module);
            if (*has_return) {
                break; // Stop if we hit a return
            }
            stmt = stmt->next;
        }
        return result;
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
