#include "codegen.h"
#include "parse.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "sokoide_module"

static LLVMValueRef codegen(Node* node, LLVMBuilderRef builder,
                            LLVMValueRef* local_allocas, bool* has_return,
                            LLVMModuleRef module);

/**
 * Converts Type to LLVMTypeRef
 */
static LLVMTypeRef to_llvm_type(Type* ty) {
    if (ty == NULL || ty->ty == INT) {
        return LLVMInt32Type();
    }
    if (ty->ty == CHAR) {
        return LLVMInt8Type();
    }
    if (ty->ty == STRUCT) {
        // Count members
        int count = 0;
        for (Member* m = ty->members; m; m = m->next)
            count++;

        LLVMTypeRef* types = malloc(sizeof(LLVMTypeRef) * count);
        int i = 0;
        for (Member* m = ty->members; m; m = m->next) {
            types[i++] = to_llvm_type(m->type);
        }
        LLVMTypeRef struct_type = LLVMStructType(types, count, false);
        free(types);
        return struct_type;
    }
    if (ty->ty == PTR) {
        if (ty->array_size > 0) {
            return LLVMArrayType(to_llvm_type(ty->ptr_to), ty->array_size);
        }
        if (ty->ptr_to == NULL) {
            return LLVMPointerType(LLVMInt32Type(), 0);
        }
        return LLVMPointerType(to_llvm_type(ty->ptr_to), 0);
    }
    return LLVMInt32Type();
}

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

    // First pass: generate global variables
    for (int i = 0; i < ctx->node_count; i++) {
        Node* node = ctx->code[i];
        if (node->kind == ND_GVAR) {
            char var_name[64];
            int len = node->tok->len < 63 ? node->tok->len : 63;
            strncpy(var_name, node->tok->str, len);
            var_name[len] = '\0';

            LLVMTypeRef var_type = to_llvm_type(node->type);
            LLVMValueRef gvar = LLVMAddGlobal(module, var_type, var_name);
            LLVMSetInitializer(gvar, LLVMConstNull(var_type));
            LLVMSetLinkage(gvar, LLVMExternalLinkage);
        }
    }

    // Generate string literal global constants
    for (int i = 0; i < ctx->string_count; i++) {
        char name[32];
        snprintf(name, sizeof(name), ".str.%d", i);
        // Create null-terminated string constant
        int str_len = ctx->string_lens[i];
        char* str_data = malloc(str_len + 1);
        memcpy(str_data, ctx->strings[i], str_len);
        str_data[str_len] = '\0';
        LLVMValueRef str_const = LLVMConstStringInContext(
            LLVMGetGlobalContext(), str_data, str_len + 1, true);
        LLVMTypeRef str_type = LLVMArrayType(LLVMInt8Type(), str_len + 1);
        LLVMValueRef gstr = LLVMAddGlobal(module, str_type, name);
        LLVMSetInitializer(gstr, str_const);
        LLVMSetGlobalConstant(gstr, true);
        LLVMSetLinkage(gstr, LLVMPrivateLinkage);
        free(str_data);
    }

    // Track if main function exists
    bool has_main = false;

    // Generate code for each function
    for (int i = 0; i < ctx->node_count; i++) {
        Node* func_node = ctx->code[i];

        if (func_node->kind != ND_FUNCTION) {
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

        // Determine return type from function node
        LLVMTypeRef ret_type;
        if (func_node->type && func_node->type->ty == 1 &&
            func_node->type->ptr_to == NULL) { // void hack
            ret_type = LLVMVoidType();
        } else {
            ret_type = to_llvm_type(func_node->type);
        }

        LLVMTypeRef* param_types = NULL;
        if (param_count > 0) {
            param_types = malloc(param_count * sizeof(LLVMTypeRef));
            param = func_node->rhs;
            for (int i = 0; i < param_count; i++) {
                param_types[i] = to_llvm_type(param->type);
                param = param->next;
            }
        }
        LLVMTypeRef func_type =
            LLVMFunctionType(ret_type, param_types, param_count, 0);
        LLVMValueRef func = LLVMAddFunction(module, func_name, func_type);

        // Create entry block
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
        LLVMPositionBuilderAtEnd(builder, entry);

        // Local variable space per function
        LLVMValueRef local_allocas[100]; // Max 100 locals for now
        memset(local_allocas, 0, sizeof(local_allocas));

        LVar* var = func_node->locals;
        while (var) {
            if (var->offset < 100) {
                char var_name[64];
                int len = var->len < 63 ? var->len : 63;
                strncpy(var_name, var->name, len);
                var_name[len] = '\0';
                LLVMTypeRef var_type = to_llvm_type(var->type);
                local_allocas[var->offset] =
                    LLVMBuildAlloca(builder, var_type, var_name);
            }
            var = var->next;
        }

        // Store parameter values into local variables
        param = func_node->rhs;
        for (int i = 0; i < param_count; i++) {
            LLVMValueRef arg = LLVMGetParam(func, i);
            if (param->val < 100 && local_allocas[param->val]) {
                LLVMBuildStore(builder, arg, local_allocas[param->val]);
            }
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
            res = codegen(stmt, builder, local_allocas, &has_return, module);
            if (has_return) {
                break;
            }
            stmt = stmt->next;
        }
        if (!has_return) {
            // For void functions, use RetVoid; for int functions, return value
            if (func_node->type && func_node->type->ty == 1 &&
                func_node->type->ptr_to == NULL) { // void hack
                LLVMBuildRetVoid(builder);
            } else {
                LLVMBuildRet(builder, res);
            }
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
                            LLVMValueRef* local_allocas, bool* has_return,
                            LLVMModuleRef module) {
    if (node == NULL) {
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }

    switch (node->kind) {
    case ND_NUM: {
        return LLVMConstInt(LLVMInt32Type(), node->val, 0);
    }
    case ND_LVAR: {
        if (node->val < 100 && local_allocas[node->val]) {
            LLVMValueRef alloca_ptr = local_allocas[node->val];
            LLVMTypeRef var_type = to_llvm_type(node->type);
            LLVMValueRef loaded =
                LLVMBuildLoad2(builder, var_type, alloca_ptr, "loadtmp");
            // Sign-extend char (i8) to int (i32) for use in expressions
            if (node->type && node->type->ty == CHAR) {
                loaded = LLVMBuildSExt(builder, loaded, LLVMInt32Type(),
                                       "sext_char");
            }
            return loaded;
        }
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_GVAR: {
        // Global variable access
        char var_name[64];
        int len = node->tok->len < 63 ? node->tok->len : 63;
        strncpy(var_name, node->tok->str, len);
        var_name[len] = '\0';

        LLVMValueRef gvar = LLVMGetNamedGlobal(module, var_name);
        if (gvar) {
            LLVMTypeRef var_type = to_llvm_type(node->type);
            LLVMValueRef loaded =
                LLVMBuildLoad2(builder, var_type, gvar, "gload");
            // Sign-extend char (i8) to int (i32) for use in expressions
            if (node->type && node->type->ty == CHAR) {
                loaded = LLVMBuildSExt(builder, loaded, LLVMInt32Type(),
                                       "sext_char");
            }
            return loaded;
        }
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_ASSIGN: {
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);

        // Truncate rhs to i8 if assigning to a char type
        LLVMValueRef store_val = rhs;
        if (node->lhs->type && node->lhs->type->ty == CHAR) {
            store_val =
                LLVMBuildTrunc(builder, rhs, LLVMInt8Type(), "trunc_char");
        }

        if (node->lhs->kind == ND_DEREF) {
            // *ptr = value - store through pointer
            LLVMValueRef ptr = codegen(node->lhs->lhs, builder, local_allocas,
                                       has_return, module);
            // Truncate if dereferenced type is char
            LLVMValueRef deref_store = rhs;
            if (node->lhs->type && node->lhs->type->ty == CHAR) {
                deref_store =
                    LLVMBuildTrunc(builder, rhs, LLVMInt8Type(), "trunc_char");
            }
            LLVMBuildStore(builder, deref_store, ptr);
        } else if (node->lhs->kind == ND_MEMBER) {
            // member access: s.a = value
            // We need the address of the member.
            // ND_ADDR handles this logic.
            Node* addr_node = new_node(ND_ADDR, node->lhs, NULL);
            addr_node->type = new_type_ptr(node->lhs->type);
            LLVMValueRef ptr =
                codegen(addr_node, builder, local_allocas, has_return, module);
            free(addr_node);

            LLVMValueRef member_store = rhs;
            if (node->lhs->type && node->lhs->type->ty == CHAR) {
                member_store =
                    LLVMBuildTrunc(builder, rhs, LLVMInt8Type(), "trunc_char");
            }
            LLVMBuildStore(builder, member_store, ptr);
        } else if (node->lhs->kind == ND_LVAR) {
            // Regular variable assignment
            if (node->lhs->val < 100 && local_allocas[node->lhs->val]) {
                LLVMValueRef alloca_ptr = local_allocas[node->lhs->val];
                LLVMBuildStore(builder, store_val, alloca_ptr);
            }
        } else if (node->lhs->kind == ND_GVAR) {
            // Global variable assignment
            char var_name[64];
            int len = node->lhs->tok->len < 63 ? node->lhs->tok->len : 63;
            strncpy(var_name, node->lhs->tok->str, len);
            var_name[len] = '\0';

            LLVMValueRef gvar = LLVMGetNamedGlobal(module, var_name);
            if (gvar) {
                LLVMBuildStore(builder, store_val, gvar);
            }
        }
        return rhs; // assignment returns the assigned value (as i32)
    }
    case ND_ADD: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);

        if (node->lhs->type && node->lhs->type->ty == PTR) {
            // ptr + int
            LLVMTypeRef elem_type = to_llvm_type(node->lhs->type->ptr_to);
            return LLVMBuildInBoundsGEP2(builder, elem_type, lhs, &rhs, 1,
                                         "ptradd");
        }
        if (node->rhs->type && node->rhs->type->ty == PTR) {
            // int + ptr
            LLVMTypeRef elem_type = to_llvm_type(node->rhs->type->ptr_to);
            return LLVMBuildInBoundsGEP2(builder, elem_type, rhs, &lhs, 1,
                                         "ptradd");
        }
        return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
    }
    case ND_SUB: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);

        if (node->lhs->type && node->lhs->type->ty == PTR) {
            if (node->rhs->type && node->rhs->type->ty == PTR) {
                // ptr - ptr
                LLVMTypeRef elem_type = to_llvm_type(node->lhs->type->ptr_to);
                return LLVMBuildPtrDiff2(builder, elem_type, lhs, rhs,
                                         "ptrdiff");
            }
            // ptr - int
            LLVMValueRef neg_rhs = LLVMBuildNeg(builder, rhs, "neg");
            LLVMTypeRef elem_type = to_llvm_type(node->lhs->type->ptr_to);
            return LLVMBuildInBoundsGEP2(builder, elem_type, lhs, &neg_rhs, 1,
                                         "ptrsub");
        }
        return LLVMBuildSub(builder, lhs, rhs, "subtmp");
    }
    case ND_MUL: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);
        return LLVMBuildMul(builder, lhs, rhs, "multmp");
    }
    case ND_DIV: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);
        return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
    }
    case ND_LT: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "lttmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_LE: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "letmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_EQ: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);
        LLVMValueRef res = LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eqtmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_NE: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);
        LLVMValueRef res = LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "netmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_GE: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "getmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_GT: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(node->rhs, builder, local_allocas, has_return, module);
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
        LLVMTypeRef* param_types = NULL;
        if (arg_count > 0) {
            args = malloc(arg_count * sizeof(LLVMValueRef));
            param_types = malloc(arg_count * sizeof(LLVMTypeRef));
            arg = node->lhs;
            for (int i = 0; i < arg_count; i++) {
                args[i] =
                    codegen(arg, builder, local_allocas, has_return, module);
                param_types[i] = LLVMTypeOf(args[i]);
                arg = arg->next;
            }
        }

        LLVMValueRef func = LLVMGetNamedFunction(module, func_name);
        LLVMTypeRef func_type;
        if (!func) {
            LLVMTypeRef i32_type = LLVMInt32Type();
            func_type = LLVMFunctionType(i32_type, param_types, arg_count, 0);
            func = LLVMAddFunction(module, func_name, func_type);
        } else {
            func_type = LLVMGlobalGetValueType(func);
        }

        LLVMValueRef result = LLVMBuildCall2(builder, func_type, func, args,
                                             arg_count, "calltmp");
        if (args) {
            free(args);
        }
        if (param_types) {
            free(param_types);
        }
        return result;
    }
    case ND_RETURN: {
        LLVMValueRef lhs =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        *has_return = true;
        return LLVMBuildRet(builder, lhs);
    }
    case ND_IF: {
        // Generate condition
        LLVMValueRef cond_val =
            codegen(node->cond, builder, local_allocas, has_return, module);

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
        codegen(node->lhs, builder, local_allocas, &then_has_return, module);
        LLVMBasicBlockRef then_end = LLVMGetInsertBlock(builder);

        // Generate else block
        LLVMPositionBuilderAtEnd(builder, else_bb);
        bool else_has_return = false;
        if (node->rhs) {
            codegen(node->rhs, builder, local_allocas, &else_has_return,
                    module);
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
            cond_val =
                codegen(node->cond, builder, local_allocas, has_return, module);
        }
        LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
        LLVMValueRef cond_bool =
            LLVMBuildICmp(builder, LLVMIntNE, cond_val, zero, "while.cond");
        LLVMBuildCondBr(builder, cond_bool, body_bb, merge_bb);

        // Generate body block
        LLVMPositionBuilderAtEnd(builder, body_bb);
        codegen(node->lhs, builder, local_allocas, has_return, module);
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
            codegen(node->init, builder, local_allocas, has_return, module);
        }

        // Jump to condition
        LLVMBuildBr(builder, cond_bb);

        // Generate condition block
        LLVMPositionBuilderAtEnd(builder, cond_bb);
        LLVMValueRef cond_val = LLVMConstInt(LLVMInt32Type(), 1, 0);
        if (node->cond) {
            cond_val =
                codegen(node->cond, builder, local_allocas, has_return, module);
        }
        LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
        LLVMValueRef cond_bool =
            LLVMBuildICmp(builder, LLVMIntNE, cond_val, zero, "for.cond");
        LLVMBuildCondBr(builder, cond_bool, body_bb, merge_bb);

        // Generate body block
        LLVMPositionBuilderAtEnd(builder, body_bb);
        codegen(node->lhs, builder, local_allocas, has_return, module);
        if (!*has_return) {
            LLVMBuildBr(builder, inc_bb);
        }

        // Generate increment block
        LLVMPositionBuilderAtEnd(builder, inc_bb);
        if (node->rhs) {
            codegen(node->rhs, builder, local_allocas, has_return, module);
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
            result = codegen(stmt, builder, local_allocas, has_return, module);
            if (*has_return) {
                break; // Stop if we hit a return
            }
            stmt = stmt->next;
        }
        return result;
    }
    case ND_DEREF: {
        // *expr - load from address
        LLVMValueRef ptr =
            codegen(node->lhs, builder, local_allocas, has_return, module);
        LLVMTypeRef loaded_type = to_llvm_type(node->type);
        LLVMValueRef loaded =
            LLVMBuildLoad2(builder, loaded_type, ptr, "deref");
        // Sign-extend char (i8) to int (i32) for use in expressions
        if (node->type && node->type->ty == CHAR) {
            loaded =
                LLVMBuildSExt(builder, loaded, LLVMInt32Type(), "sext_char");
        }
        return loaded;
    }
    case ND_MEMBER: {
        // member access: s.a
        Node* addr_node = new_node(ND_ADDR, node, NULL);
        addr_node->type = new_type_ptr(node->type);
        LLVMValueRef ptr =
            codegen(addr_node, builder, local_allocas, has_return, module);
        free(addr_node);

        LLVMTypeRef member_type = to_llvm_type(node->type);
        LLVMValueRef loaded =
            LLVMBuildLoad2(builder, member_type, ptr, "mload");
        // Sign-extend char (i8) to int (i32) for use in expressions
        if (node->type && node->type->ty == CHAR) {
            loaded =
                LLVMBuildSExt(builder, loaded, LLVMInt32Type(), "sext_char");
        }
        return loaded;
    }
    case ND_ADDR: {
        // &expr - get address of variable
        if (node->lhs->kind == ND_LVAR) {
            if (node->lhs->val < 100 && local_allocas[node->lhs->val]) {
                LLVMValueRef ptr = local_allocas[node->lhs->val];
                if (node->lhs->type->array_size > 0) {
                    // Implicit conversion or &a
                    LLVMValueRef indices[] = {
                        LLVMConstInt(LLVMInt32Type(), 0, false),
                        LLVMConstInt(LLVMInt32Type(), 0, false)};
                    LLVMTypeRef array_type = to_llvm_type(node->lhs->type);
                    return LLVMBuildInBoundsGEP2(builder, array_type, ptr,
                                                 indices, 2, "array_to_ptr");
                }
                return ptr;
            }
        } else if (node->lhs->kind == ND_MEMBER) {
            // &s.a
            // Get base address
            LLVMValueRef base_addr;
            if (node->lhs->lhs->kind == ND_LVAR) {
                base_addr = local_allocas[node->lhs->lhs->val];
            } else if (node->lhs->lhs->kind == ND_GVAR) {
                // handle global struct if needed
                char var_name[64];
                int len = node->lhs->lhs->tok->len < 63
                              ? node->lhs->lhs->tok->len
                              : 63;
                strncpy(var_name, node->lhs->lhs->tok->str, len);
                var_name[len] = '\0';
                base_addr = LLVMGetNamedGlobal(module, var_name);
            } else if (node->lhs->lhs->kind == ND_MEMBER) {
                // recursive nested struct: &s.a.b
                Node* nested_addr = new_node(ND_ADDR, node->lhs->lhs, NULL);
                nested_addr->type = new_type_ptr(node->lhs->lhs->type);
                base_addr = codegen(nested_addr, builder, local_allocas,
                                    has_return, module);
                free(nested_addr);
            } else {
                fprintf(stderr, "Unsupported base for member access\n");
                exit(1);
            }

            LLVMTypeRef struct_type = to_llvm_type(node->lhs->lhs->type);
            return LLVMBuildStructGEP2(builder, struct_type, base_addr,
                                       node->lhs->member->index, "mgep");
        } else if (node->lhs->kind == ND_GVAR) {
            char var_name[64];
            int len = node->lhs->tok->len < 63 ? node->lhs->tok->len : 63;
            strncpy(var_name, node->lhs->tok->str, len);
            var_name[len] = '\0';

            LLVMValueRef gvar = LLVMGetNamedGlobal(module, var_name);
            if (gvar) {
                if (node->lhs->type && node->lhs->type->array_size > 0) {
                    LLVMValueRef indices[] = {
                        LLVMConstInt(LLVMInt32Type(), 0, false),
                        LLVMConstInt(LLVMInt32Type(), 0, false)};
                    LLVMTypeRef array_type = to_llvm_type(node->lhs->type);
                    return LLVMBuildInBoundsGEP2(builder, array_type, gvar,
                                                 indices, 2, "garray_to_ptr");
                }
                return gvar;
            }
        }
        // For other nodes, return null for now
        return LLVMConstPointerNull(LLVMPointerType(LLVMInt32Type(), 0));
    }
    case ND_DECL: {
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_STR: {
        // Get the global string constant by name
        char name[32];
        snprintf(name, sizeof(name), ".str.%d", node->val);
        LLVMValueRef gstr = LLVMGetNamedGlobal(module, name);
        if (gstr) {
            // GEP to get pointer to first element (i8*)
            LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, false),
                                      LLVMConstInt(LLVMInt32Type(), 0, false)};
            LLVMTypeRef str_type = LLVMGlobalGetValueType(gstr);
            return LLVMBuildInBoundsGEP2(builder, str_type, gstr, indices, 2,
                                         "str_ptr");
        }
        return LLVMConstPointerNull(LLVMPointerType(LLVMInt8Type(), 0));
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
