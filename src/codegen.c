#include "codegen.h"
#include "parse.h"
#include <llvm-c/Analysis.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "sokoide_module"

static LLVMValueRef codegen(Context* ctx, Node* node, LLVMBuilderRef builder,
                            LLVMValueRef* local_allocas, bool* has_return,
                            LLVMModuleRef module);

/**
 * Converts Type to LLVMTypeRef
 */
static LLVMTypeRef to_llvm_type(Type* ty) {
    if (ty == NULL || ty->ty == INT) {
        return LLVMInt32Type();
    }
    if (ty->ty == VOID) {
        return LLVMVoidType();
    }
    if (ty->ty == CHAR) {
        return LLVMInt8Type();
    }
    if (ty->ty == LONG) {
        return LLVMInt64Type();
    }
    if (ty->ty == STRUCT) {
        // Check cache first
        if (ty->llvm_type) {
            return (LLVMTypeRef)ty->llvm_type;
        }

        // Create a named struct type to handle recursive types
        LLVMTypeRef named_struct =
            LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.anon");
        ty->llvm_type = named_struct;

        // Count members
        int count = 0;
        for (Member* m = ty->members; m; m = m->next)
            count++;

        if (count > 0) {
            LLVMTypeRef* types = malloc(sizeof(LLVMTypeRef) * count);
            int i = 0;
            for (Member* m = ty->members; m; m = m->next) {
                types[i++] = to_llvm_type(m->type);
            }
            LLVMStructSetBody(named_struct, types, count, false);
            free(types);
        }
        return named_struct;
    }
    if (ty->ty == PTR) {
        if (ty->array_size > 0) {
            return LLVMArrayType(to_llvm_type(ty->ptr_to), ty->array_size);
        }
        if (ty->ptr_to == NULL || ty->ptr_to->ty == VOID) {
            return LLVMPointerType(LLVMInt8Type(), 0);
        }
        return LLVMPointerType(to_llvm_type(ty->ptr_to), 0);
    }
    return LLVMInt32Type();
}

// Helper to match types for binary operations
static void match_types(LLVMBuilderRef builder, LLVMValueRef* lhs,
                        LLVMValueRef* rhs) {
    LLVMTypeRef lty = LLVMTypeOf(*lhs);
    LLVMTypeRef rty = LLVMTypeOf(*rhs);

    if (lty == rty)
        return;

    LLVMTypeKind lk = LLVMGetTypeKind(lty);
    LLVMTypeKind rk = LLVMGetTypeKind(rty);

    if (lk == LLVMIntegerTypeKind && rk == LLVMPointerTypeKind) {
        *lhs = LLVMBuildIntToPtr(builder, *lhs, rty, "inttoptr");
    } else if (lk == LLVMPointerTypeKind && rk == LLVMIntegerTypeKind) {
        *rhs = LLVMBuildIntToPtr(builder, *rhs, lty, "inttoptr");
    } else if (lk == LLVMIntegerTypeKind && rk == LLVMIntegerTypeKind) {
        // Integer promotion/truncation
        unsigned lw = LLVMGetIntTypeWidth(lty);
        unsigned rw = LLVMGetIntTypeWidth(rty);
        if (lw < rw)
            *lhs = LLVMBuildSExt(builder, *lhs, rty, "sext");
        else if (lw > rw)
            *rhs = LLVMBuildSExt(builder, *rhs, lty, "sext");
    }
}

static LLVMValueRef cast_value(LLVMBuilderRef builder, LLVMValueRef val,
                               LLVMTypeRef dest_ty) {
    LLVMTypeRef src_ty = LLVMTypeOf(val);
    if (src_ty == dest_ty)
        return val;

    LLVMTypeKind src_kind = LLVMGetTypeKind(src_ty);
    LLVMTypeKind dest_kind = LLVMGetTypeKind(dest_ty);

    if (dest_kind == LLVMPointerTypeKind && src_kind == LLVMIntegerTypeKind) {
        return LLVMBuildIntToPtr(builder, val, dest_ty, "cast_itop");
    }
    if (dest_kind == LLVMIntegerTypeKind && src_kind == LLVMPointerTypeKind) {
        return LLVMBuildPtrToInt(builder, val, dest_ty, "cast_ptoi");
    }
    if (dest_kind == LLVMIntegerTypeKind && src_kind == LLVMIntegerTypeKind) {
        unsigned src_w = LLVMGetIntTypeWidth(src_ty);
        unsigned dest_w = LLVMGetIntTypeWidth(dest_ty);
        if (src_w < dest_w)
            return LLVMBuildSExt(builder, val, dest_ty, "cast_sext");
        if (src_w > dest_w)
            return LLVMBuildTrunc(builder, val, dest_ty, "cast_trunc");
    }
    if (dest_kind == LLVMPointerTypeKind && src_kind == LLVMPointerTypeKind) {
        return LLVMBuildBitCast(builder, val, dest_ty, "cast_bitcast");
    }
    return val;
}

// Helper to convert value to i1 boolean for conditions
static LLVMValueRef convert_to_bool(LLVMBuilderRef builder, LLVMValueRef val) {
    LLVMTypeRef ty = LLVMTypeOf(val);
    if (LLVMGetTypeKind(ty) == LLVMPointerTypeKind) {
        return LLVMBuildIsNotNull(builder, val, "ptr_bool");
    }
    if (LLVMGetTypeKind(ty) == LLVMIntegerTypeKind) {
        LLVMValueRef zero = LLVMConstInt(ty, 0, false);
        return LLVMBuildICmp(builder, LLVMIntNE, val, zero, "int_bool");
    }
    return val;
}

/**
 * Internal function: Generates LLVM IR module from a Node AST
 *
 * @param[in] ctx Context containing AST nodes
 * @return LLVMModuleRef Reference to generated LLVM module
 */
static LLVMValueRef codegen_constant(Node* node, LLVMModuleRef module) {
    if (node->kind == ND_NUM) {
        return LLVMConstInt(to_llvm_type(node->type), node->val, 1);
    }
    if (node->kind == ND_STR) {
        char name[32];
        snprintf(name, sizeof(name), ".str.%d", node->val);
        LLVMValueRef gstr = LLVMGetNamedGlobal(module, name);
        if (!gstr)
            return LLVMConstNull(to_llvm_type(node->type));
        LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0),
                                  LLVMConstInt(LLVMInt32Type(), 0, 0)};
        return LLVMConstInBoundsGEP2(LLVMGlobalGetValueType(gstr), gstr,
                                     indices, 2);
    }
    if (node->kind == ND_INIT) {
        Type* elem_type = node->type->ptr_to;
        LLVMTypeRef llvm_elem_type = to_llvm_type(elem_type);

        int count = 0;
        for (Node* n = node->lhs; n; n = n->next)
            count++;

        LLVMValueRef* values = calloc(count, sizeof(LLVMValueRef));
        int i = 0;
        for (Node* n = node->lhs; n; n = n->next) {
            values[i++] = codegen_constant(n, module);
        }

        LLVMValueRef ret = LLVMConstArray(llvm_elem_type, values, count);
        free(values);
        return ret;
    }

    if (node->kind == ND_CAST) {
        return codegen_constant(node->lhs, module);
    }
    if (node->kind == ND_ADDR) {
        if (node->lhs->kind == ND_GVAR) {
            char var_name[64];
            int len = node->lhs->tok->len < 63 ? node->lhs->tok->len : 63;
            strncpy(var_name, node->lhs->tok->str, len);
            var_name[len] = '\0';
            return LLVMGetNamedGlobal(module, var_name);
        }
    }
    return LLVMConstNull(to_llvm_type(node->type));
}

LLVMModuleRef generate_module(Context* ctx) {
    // Create a new LLVM module with specified name
    LLVMModuleRef module = LLVMModuleCreateWithName(MODULE_NAME);
    // Create an LLVM builder for constructing instructions
    // Create an LLVM builder for constructing instructions
    LLVMBuilderRef builder = LLVMCreateBuilder();

    // Declare standard library functions
    LLVMTypeRef i8_ptr_type = LLVMPointerType(LLVMInt8Type(), 0);
    LLVMTypeRef i32_type = LLVMInt32Type();
    LLVMTypeRef i64_type = LLVMInt64Type(); // size_t on 64-bit

    // int printf(const char*, ...)
    LLVMTypeRef printf_args[] = {i8_ptr_type};
    LLVMAddFunction(module, "printf",
                    LLVMFunctionType(i32_type, printf_args, 1, true));

    // int fprintf(FILE*, const char*, ...)
    LLVMTypeRef fprintf_args[] = {i8_ptr_type, i8_ptr_type}; // FILE* as i8*
    LLVMAddFunction(module, "fprintf",
                    LLVMFunctionType(i32_type, fprintf_args, 2, true));

    // void exit(int)
    LLVMTypeRef exit_args[] = {i32_type};
    LLVMAddFunction(module, "exit",
                    LLVMFunctionType(LLVMVoidType(), exit_args, 1, false));

    // void* malloc(size_t)
    LLVMTypeRef malloc_args[] = {i64_type};
    LLVMAddFunction(module, "malloc",
                    LLVMFunctionType(i8_ptr_type, malloc_args, 1, false));

    // void* calloc(size_t, size_t)
    LLVMTypeRef calloc_args[] = {i64_type, i64_type};
    LLVMAddFunction(module, "calloc",
                    LLVMFunctionType(i8_ptr_type, calloc_args, 2, false));

    // void* realloc(void*, size_t)
    LLVMTypeRef realloc_args[] = {i8_ptr_type, i64_type};
    LLVMAddFunction(module, "realloc",
                    LLVMFunctionType(i8_ptr_type, realloc_args, 2, false));

    // void free(void*)
    LLVMTypeRef free_args[] = {i8_ptr_type};
    LLVMAddFunction(module, "free",
                    LLVMFunctionType(LLVMVoidType(), free_args, 1, false));

    // long strtol(const char*, char**, int)
    LLVMTypeRef strtol_args[] = {i8_ptr_type, LLVMPointerType(i8_ptr_type, 0),
                                 i32_type};
    LLVMAddFunction(module, "strtol",
                    LLVMFunctionType(i32_type, strtol_args, 3, false));

    // size_t strlen(const char*)
    LLVMTypeRef strlen_args[] = {i8_ptr_type};
    LLVMAddFunction(module, "strlen",
                    LLVMFunctionType(i64_type, strlen_args, 1, false));

    // int strncmp(const char*, const char*, size_t)
    LLVMTypeRef strncmp_args[] = {i8_ptr_type, i8_ptr_type, i64_type};
    LLVMAddFunction(module, "strncmp",
                    LLVMFunctionType(i32_type, strncmp_args, 3, false));

    // char* strncpy(char*, const char*, size_t)
    LLVMTypeRef strncpy_args[] = {i8_ptr_type, i8_ptr_type, i64_type};
    LLVMAddFunction(module, "strncpy",
                    LLVMFunctionType(i8_ptr_type, strncpy_args, 3, false));

    // int memcmp(const void*, const void*, size_t)
    LLVMTypeRef memcmp_args[] = {i8_ptr_type, i8_ptr_type, i64_type};
    LLVMAddFunction(module, "memcmp",
                    LLVMFunctionType(i32_type, memcmp_args, 3, false));

    // void* memcpy(void*, const void*, size_t)
    LLVMTypeRef memcpy_args[] = {i8_ptr_type, i8_ptr_type, i64_type};
    LLVMAddFunction(module, "memcpy",
                    LLVMFunctionType(i8_ptr_type, memcpy_args, 3, false));

    // void alloc4(int**, int, int, int, int) - test helper function
    LLVMTypeRef alloc4_args[] = {LLVMPointerType(i8_ptr_type, 0), i32_type,
                                 i32_type, i32_type, i32_type};
    LLVMAddFunction(module, "alloc4",
                    LLVMFunctionType(LLVMVoidType(), alloc4_args, 5, false));

    // First pass: generate string literal global constants
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

    // Second pass: generate global variables
    for (int i = 0; i < ctx->node_count; i++) {
        Node* node = ctx->code[i];
        if (node->kind == ND_GVAR) {
            char var_name[64];
            int len = node->tok->len < 63 ? node->tok->len : 63;
            strncpy(var_name, node->tok->str, len);
            var_name[len] = '\0';

            LLVMTypeRef var_type = to_llvm_type(node->type);
            LLVMValueRef gvar = LLVMAddGlobal(module, var_type, var_name);
            if (node->init) {
                LLVMSetInitializer(gvar, codegen_constant(node->init, module));
            } else if (!node->is_extern) {
                // Non-extern globals without initializer get null init
                LLVMSetInitializer(gvar, LLVMConstNull(var_type));
            }
            // extern globals have no initializer (declaration only)
            if (node->is_extern) {
                LLVMSetLinkage(gvar, LLVMExternalLinkage);
            }
        }
    }

    // Generate code for each function
    for (int i = 0; i < ctx->node_count; i++) {
        Node* func_node = ctx->code[i];

        if (func_node->kind != ND_FUNCTION) {
            continue;
        }

        // Set current function return type in context
        if (func_node->type) {
            ctx->current_func_type = func_node->type;
        } else {
            ctx->current_func_type = NULL;
        }

        // Get function name
        char func_name[64];
        int len = func_node->tok->len < 63 ? func_node->tok->len : 63;
        strncpy(func_name, func_node->tok->str, len);
        func_name[len] = '\0';

        // Count parameters (excluding ellipsis)
        int param_count = 0;
        bool is_variadic = func_node->is_vararg;
        Node* param = func_node->rhs;
        while (param) {
            param_count++;
            param = param->next;
        }

        // Determine return type from function node
        LLVMTypeRef ret_type;
        if (func_node->type && func_node->type->ty == VOID &&
            func_node->type->ptr_to == NULL) {
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
        LLVMTypeRef func_type = LLVMFunctionType(
            ret_type, param_types, param_count, is_variadic ? 1 : 0);

        LLVMValueRef func = LLVMGetNamedFunction(module, func_name);
        if (!func) {
            func = LLVMAddFunction(module, func_name, func_type);
        }

        // If it's a prototype (no body), skip building the body
        if (func_node->lhs == NULL) {
            if (param_types)
                free(param_types);
            continue;
        }

        // Create entry block
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
        LLVMPositionBuilderAtEnd(builder, entry);

        // Local variable space per function
        LLVMValueRef local_allocas[1024]; // Max 100 locals for now
        memset(local_allocas, 0, sizeof(local_allocas));

        LVar* var = func_node->locals;
        while (var) {
            if (var->offset < 1024) {
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
            if (param->val < 1024 && local_allocas[param->val]) {
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
            res =
                codegen(ctx, stmt, builder, local_allocas, &has_return, module);
            if (has_return) {
                break;
            }
            stmt = stmt->next;
        }
        if (!has_return) {
            // Generate appropriate return based on function return type
            if (func_node->type && func_node->type->ty == VOID &&
                func_node->type->ptr_to == NULL) { // void hack
                LLVMBuildRetVoid(builder);
            } else {
                // For non-void functions, return a default value
                LLVMTypeRef ret_type = to_llvm_type(func_node->type);
                LLVMValueRef default_val;
                if (func_node->type && func_node->type->ty == PTR) {
                    default_val = LLVMConstNull(ret_type); // null pointer
                } else {
                    default_val = LLVMConstInt(ret_type, 0, 0); // 0
                }
                LLVMBuildRet(builder, default_val);
            }
        }
    }

    // Removed mandatory main check to allow library compilation
    char* error = NULL;
    if (LLVMVerifyModule(module, LLVMReturnStatusAction, &error)) {
        fprintf(stderr, "LLVM IR verification failed: %s\n", error);
        LLVMDisposeMessage(error);
    }

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
static LLVMValueRef codegen(Context* ctx, Node* node, LLVMBuilderRef builder,
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
        if (node->val < 1024 && local_allocas[node->val]) {
            LLVMValueRef alloca_ptr = local_allocas[node->val];
            if (node->type && node->type->array_size > 0) {
                return alloca_ptr;
            }
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
            if (node->type && node->type->array_size > 0) {
                return gvar;
            }
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
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);

        LLVMValueRef store_val =
            cast_value(builder, rhs, to_llvm_type(node->lhs->type));

        if (node->lhs->kind == ND_DEREF) {
            // *ptr = value - store through pointer
            LLVMValueRef ptr = codegen(ctx, node->lhs->lhs, builder,
                                       local_allocas, has_return, module);
            LLVMBuildStore(builder, store_val, ptr);
        } else if (node->lhs->kind == ND_MEMBER) {
            // member access: s.a = value
            Node* addr_node = new_node(ND_ADDR, node->lhs, NULL);
            addr_node->type = new_type_ptr(node->lhs->type);
            LLVMValueRef ptr = codegen(ctx, addr_node, builder, local_allocas,
                                       has_return, module);
            free(addr_node);

            LLVMBuildStore(builder, store_val, ptr);
        } else if (node->lhs->kind == ND_LVAR) {
            // Regular variable assignment
            if (node->lhs->val < 1024 && local_allocas[node->lhs->val]) {
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
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);

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
        match_types(builder, &lhs, &rhs);
        return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
    }
    case ND_SUB: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);

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
        match_types(builder, &lhs, &rhs);
        return LLVMBuildSub(builder, lhs, rhs, "subtmp");
    }
    case ND_MUL: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        match_types(builder, &lhs, &rhs);
        return LLVMBuildMul(builder, lhs, rhs, "multmp");
    }
    case ND_DIV: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        match_types(builder, &lhs, &rhs);
        return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
    }
    case ND_MOD: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        match_types(builder, &lhs, &rhs);
        return LLVMBuildSRem(builder, lhs, rhs, "modtmp");
    }
    case ND_LT: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        match_types(builder, &lhs, &rhs);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "lttmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_LE: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        match_types(builder, &lhs, &rhs);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "letmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_EQ: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        match_types(builder, &lhs, &rhs);
        LLVMValueRef res = LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eqtmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_NE: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        match_types(builder, &lhs, &rhs);
        LLVMValueRef res = LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "netmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_GE: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        match_types(builder, &lhs, &rhs);
        LLVMValueRef res =
            LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "getmp");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "zexttmp");
    }
    case ND_GT: {
        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        match_types(builder, &lhs, &rhs);
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
                args[i] = codegen(ctx, arg, builder, local_allocas, has_return,
                                  module);
                param_types[i] = LLVMTypeOf(args[i]);
                arg = arg->next;
            }
        }

        LLVMValueRef func = LLVMGetNamedFunction(module, func_name);
        LLVMTypeRef func_type;
        LLVMTypeRef* dest_param_types = NULL;
        int dest_param_count = 0;

        if (!func) {
            LLVMTypeRef i32_type = LLVMInt32Type();
            // Default to varargs to avoid argument count mismatch for
            // undeclared functions
            func_type = LLVMFunctionType(i32_type, param_types, arg_count,
                                         1); // Variadic=true
            func = LLVMAddFunction(module, func_name, func_type);

            // Register function type for calls
            FuncType* ft = calloc(1, sizeof(FuncType));
            ft->name = strdup(func_name);
            ft->len = strlen(func_name);
            ft->llvm_type = func_type;
            ft->next = ctx->func_types;
            ctx->func_types = ft;
        } else {
            // Lookup function type in our context
            func_type = NULL;
            for (FuncType* ft = ctx->func_types; ft; ft = ft->next) {
                if (strlen(func_name) == (size_t)ft->len &&
                    strncmp(func_name, ft->name, ft->len) == 0) {
                    func_type = ft->llvm_type;
                    break;
                }
            }

            if (!func_type) {
                // Fallback for external functions not seen yet?
                // Or try global value type (might start working if not opaque)
                func_type = LLVMGlobalGetValueType(func);
                // Maybe it returns PointerType, let's try to handle it if so
                if (LLVMGetTypeKind(func_type) == LLVMPointerTypeKind) {
                    // Try GetElementType if reachable
                    // func_type = LLVMGetElementType(func_type);
                    // If opaque, we are stuck without declaration.
                    // But printf should be declared in main.i
                }
            }

            // Debug
            // fprintf(stderr, "DEBUG: call %s type=%d vararg=%d\n", func_name,
            // LLVMGetTypeKind(func_type), (func_type &&
            // LLVMGetTypeKind(func_type)==LLVMFunctionTypeKind) ?
            // LLVMIsFunctionVarArg(func_type) : -1);

            dest_param_count = LLVMCountParamTypes(func_type);
            if (dest_param_count > 0) {
                dest_param_types =
                    malloc(sizeof(LLVMTypeRef) * dest_param_count);
                LLVMGetParamTypes(func_type, dest_param_types);
            }
        }

        // Cast arguments if possible/necessary
        if (dest_param_types && arg_count > 0) {
            for (int i = 0; i < arg_count; i++) {
                if (i >= dest_param_count && LLVMIsFunctionVarArg(func_type)) {
                    // Varargs part, use implicit casts rules or leave as is?
                    // For printf/fprintf, we often need to promote?
                    // But here just handling basic ptr/int mismatch
                    // Usually varargs don't have target types, so we rely on
                    // codegen outputs.
                    break;
                }
                if (i >= dest_param_count)
                    break;

                LLVMTypeRef dest_ty = dest_param_types[i];
                args[i] = cast_value(builder, args[i], dest_ty);
            }
            if (dest_param_types)
                free(dest_param_types);
        }

        LLVMTypeRef ret_type = LLVMGetReturnType(func_type);
        LLVMValueRef result;
        if (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind) {
            result =
                LLVMBuildCall2(builder, func_type, func, args, arg_count, "");
        } else {
            result = LLVMBuildCall2(builder, func_type, func, args, arg_count,
                                    "calltmp");
        }

        if (args)
            free(args);
        if (param_types)
            free(param_types);
        return result;
    }
    case ND_PRE_INC:
    case ND_POST_INC:
    case ND_PRE_DEC:
    case ND_POST_DEC: {
        // Get address of operand
        Node* addr_node = new_node(ND_ADDR, node->lhs, NULL);
        addr_node->type = new_type_ptr(node->lhs->type);
        LLVMValueRef ptr =
            codegen(ctx, addr_node, builder, local_allocas, has_return, module);
        free(addr_node);

        if (!ptr) {
            fprintf(stderr, "incdec: pointer is NULL\n");
            exit(1);
        }

        // Load current value
        LLVMTypeRef val_type = to_llvm_type(node->lhs->type);
        LLVMValueRef old_val =
            LLVMBuildLoad2(builder, val_type, ptr, "incdec.old");
        LLVMValueRef new_val;

        if (node->lhs->type && node->lhs->type->ty == PTR) {
            // Pointer arithmetic
            LLVMTypeRef ptr_to_type = to_llvm_type(node->lhs->type->ptr_to);
            LLVMValueRef idx = LLVMConstInt(LLVMInt32Type(), 1, 0);
            if (node->kind == ND_PRE_DEC || node->kind == ND_POST_DEC) {
                idx = LLVMConstInt(LLVMInt32Type(), -1, 1);
            }
            new_val = LLVMBuildInBoundsGEP2(builder, ptr_to_type, old_val, &idx,
                                            1, "incdec.ptr");
        } else {
            // Integer arithmetic
            LLVMValueRef one = LLVMConstInt(LLVMInt32Type(), 1, 0);
            match_types(builder, &old_val, &one);
            if (node->kind == ND_PRE_INC || node->kind == ND_POST_INC) {
                new_val = LLVMBuildAdd(builder, old_val, one, "incdec.new");
            } else {
                new_val = LLVMBuildSub(builder, old_val, one, "incdec.new");
            }
        }

        // Store back
        LLVMBuildStore(builder, new_val, ptr);

        // Return old or new value
        if (node->kind == ND_PRE_INC || node->kind == ND_PRE_DEC) {
            return new_val;
        } else {
            return old_val;
        }
    }

    case ND_IF: {
        // Generate condition
        LLVMValueRef cond_val = codegen(ctx, node->cond, builder, local_allocas,
                                        has_return, module);

        // Comparer with 0 (condition != 0)
        LLVMValueRef cond_bool = convert_to_bool(builder, cond_val);

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
        codegen(ctx, node->lhs, builder, local_allocas, &then_has_return,
                module);
        LLVMBasicBlockRef then_end = LLVMGetInsertBlock(builder);

        // Generate else block
        LLVMPositionBuilderAtEnd(builder, else_bb);
        bool else_has_return = false;
        if (node->rhs) {
            codegen(ctx, node->rhs, builder, local_allocas, &else_has_return,
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

        void* old_break = ctx->current_break_label;
        void* old_continue = ctx->current_continue_label;
        ctx->current_break_label = merge_bb;
        ctx->current_continue_label = cond_bb;

        // Jump to condition
        LLVMBuildBr(builder, cond_bb);

        // Generate condition block
        LLVMPositionBuilderAtEnd(builder, cond_bb);
        LLVMValueRef cond_val = LLVMConstInt(LLVMInt32Type(), 1, 0);
        if (node->cond) {
            cond_val = codegen(ctx, node->cond, builder, local_allocas,
                               has_return, module);
        }
        LLVMValueRef cond_bool = convert_to_bool(builder, cond_val);
        LLVMBuildCondBr(builder, cond_bool, body_bb, merge_bb);

        // Generate body block
        LLVMPositionBuilderAtEnd(builder, body_bb);
        bool body_has_return = false;
        codegen(ctx, node->lhs, builder, local_allocas, &body_has_return,
                module);
        if (!body_has_return) {
            LLVMBuildBr(builder, cond_bb); // Loop back
        }

        // Position at merge block
        LLVMPositionBuilderAtEnd(builder, merge_bb);

        ctx->current_break_label = old_break;
        ctx->current_continue_label = old_continue;
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_FOR: {
        // Check if this is an infinite loop (for (;;))
        bool is_infinite = (node->cond == NULL);

        // Create basic blocks: cond, body, inc, merge
        LLVMBasicBlockRef cond_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "for.cond");
        LLVMBasicBlockRef body_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "for.body");
        LLVMBasicBlockRef inc_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "for.inc");
        LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "for.end");

        void* old_break = ctx->current_break_label;
        void* old_continue = ctx->current_continue_label;
        ctx->current_break_label = merge_bb;
        ctx->current_continue_label = inc_bb;
        // Generate init if present
        if (node->init) {
            codegen(ctx, node->init, builder, local_allocas, has_return,
                    module);
        }

        // Jump to condition
        LLVMBuildBr(builder, cond_bb);

        // Generate condition block
        LLVMPositionBuilderAtEnd(builder, cond_bb);
        LLVMValueRef cond_val = LLVMConstInt(LLVMInt32Type(), 1, 0);
        if (node->cond) {
            cond_val = codegen(ctx, node->cond, builder, local_allocas,
                               has_return, module);
        }
        LLVMValueRef cond_bool = convert_to_bool(builder, cond_val);

        // For infinite loops, don't create a branch to merge_bb
        if (is_infinite) {
            LLVMBuildBr(builder, body_bb);
        } else {
            LLVMBuildCondBr(builder, cond_bool, body_bb, merge_bb);
        }

        // Generate body block
        LLVMPositionBuilderAtEnd(builder, body_bb);
        bool body_returns = false;
        codegen(ctx, node->lhs, builder, local_allocas, &body_returns, module);
        if (!body_returns &&
            LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder)) == NULL) {
            LLVMBuildBr(builder, inc_bb);
        }

        // Generate increment block
        LLVMPositionBuilderAtEnd(builder, inc_bb);
        if (node->rhs) {
            bool inc_has_return = false;
            codegen(ctx, node->rhs, builder, local_allocas, &inc_has_return,
                    module);
        }
        LLVMBuildBr(builder, cond_bb); // Loop back to condition

        // Position at merge block
        LLVMPositionBuilderAtEnd(builder, merge_bb);

        ctx->current_break_label = old_break;
        ctx->current_continue_label = old_continue;
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_SWITCH: {
        LLVMValueRef cond = codegen(ctx, node->cond, builder, local_allocas,
                                    has_return, module);
        LLVMBasicBlockRef break_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "sw_break");

        // Count cases
        int case_count = 0;
        for (Node* c = node->cases; c; c = c->next_case)
            case_count++;

        LLVMValueRef sw_inst =
            LLVMBuildSwitch(builder, cond, break_bb, case_count);

        LLVMValueRef old_sw_inst = (LLVMValueRef)ctx->current_switch_inst;
        void* old_break = ctx->current_break_label;
        void* old_continue =
            ctx->current_continue_label; // Save continue label for nested loops

        ctx->current_switch_inst = sw_inst;
        ctx->current_break_label = break_bb;
        ctx->current_continue_label = NULL; // No continue in switch

        if (node->lhs && node->lhs->kind == ND_BLOCK) {
            for (Node* stmt = node->lhs->lhs; stmt; stmt = stmt->next) {
                bool stmt_has_return = false;
                codegen(ctx, stmt, builder, local_allocas, &stmt_has_return,
                        module);
            }
        } else {
            bool stmt_has_return = false;
            codegen(ctx, node->lhs, builder, local_allocas, &stmt_has_return,
                    module);
        }

        // Terminate the last block of the switch if not already terminated
        LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
        if (LLVMGetBasicBlockTerminator(current_block) == NULL) {
            LLVMBuildBr(builder, break_bb);
        }

        ctx->current_switch_inst = old_sw_inst;
        ctx->current_break_label = old_break;
        ctx->current_continue_label = old_continue; // Restore continue label
        *has_return = false;

        LLVMPositionBuilderAtEnd(builder, break_bb);
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_CASE: {
        LLVMBasicBlockRef case_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "sw_case");

        // Fall through from previous block
        LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
        if (LLVMGetBasicBlockTerminator(current_block) == NULL) {
            LLVMBuildBr(builder, case_bb);
        }

        if (node->is_default) {
            LLVMSetOperand((LLVMValueRef)ctx->current_switch_inst, 1,
                           LLVMBasicBlockAsValue(case_bb));
        } else {
            int case_value = node->case_val;
            if (case_value == 0 && node->val != 0) {
                case_value = node->val;
            }
            LLVMAddCase((LLVMValueRef)ctx->current_switch_inst,
                        LLVMConstInt(LLVMInt32Type(), case_value, 0), case_bb);
        }

        LLVMPositionBuilderAtEnd(builder, case_bb);
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_BREAK: // break
        if (!ctx->current_break_label) {
            fprintf(stderr, "break outside loop/switch\n");
            exit(1);
        }
        LLVMBuildBr(builder, (LLVMBasicBlockRef)ctx->current_break_label);
        // Following code is unreachable
        LLVMBasicBlockRef dead_bb = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)),
            "unreachable");
        LLVMPositionBuilderAtEnd(builder, dead_bb);
        return LLVMConstInt(LLVMInt32Type(), 0, 0);

    case ND_CONTINUE: // continue
        if (!ctx->current_continue_label) {
            fprintf(stderr, "continue outside loop\n");
            exit(1);
        }
        LLVMBuildBr(builder, (LLVMBasicBlockRef)ctx->current_continue_label);
        // Following code is unreachable
        LLVMBasicBlockRef dead_bb_cont = LLVMAppendBasicBlock(
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)),
            "unreachable");
        LLVMPositionBuilderAtEnd(builder, dead_bb_cont);
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    case ND_BLOCK: {
        // Execute statements in sequence
        LLVMValueRef result = LLVMConstInt(LLVMInt32Type(), 0, 0);
        Node* stmt = node->lhs;
        while (stmt != NULL) {
            result =
                codegen(ctx, stmt, builder, local_allocas, has_return, module);
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
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
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
            codegen(ctx, addr_node, builder, local_allocas, has_return, module);
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
    case ND_ARRAY_TO_PTR:
    case ND_ADDR: {
        // &expr - get address of variable
        if (node->lhs->kind == ND_LVAR) {
            if (node->lhs->val < 1024 && local_allocas[node->lhs->val]) {
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
                base_addr = codegen(ctx, nested_addr, builder, local_allocas,
                                    has_return, module);
                free(nested_addr);
            } else if (node->lhs->lhs->kind == ND_DEREF) {
                // p->m: base is (*p), we need p's value as address
                base_addr = codegen(ctx, node->lhs->lhs->lhs, builder,
                                    local_allocas, has_return, module);
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
        } else if (node->lhs->kind == ND_DEREF) {
            return codegen(ctx, node->lhs->lhs, builder, local_allocas,
                           has_return, module);
        }
        // For other nodes, return null for now
        return LLVMConstPointerNull(LLVMPointerType(LLVMInt32Type(), 0));
    }
    case ND_CAST: {
        LLVMValueRef val =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        return cast_value(builder, val, to_llvm_type(node->type));
    }
    case ND_DECL: {
        if (node->init) {
            if (node->val < 0 || node->val >= 1024) {
                fprintf(stderr, "ND_DECL: node->val %d out of bounds\n",
                        node->val);
                fflush(stderr);
                exit(1);
            }
            LLVMValueRef alloca_ptr = local_allocas[node->val];
            if (!alloca_ptr) {
                fprintf(stderr, "ND_DECL: alloca_ptr is NULL for val %d\n",
                        node->val);
                fflush(stderr);
                exit(1);
            }

            if (node->init->kind == ND_INIT) {
                // Handle array/struct initialization
                Node* cur = node->init->lhs;
                int i = 0;
                LLVMTypeRef var_type = to_llvm_type(node->type);

                while (cur) {
                    LLVMValueRef val = codegen(ctx, cur, builder, local_allocas,
                                               has_return, module);
                    LLVMValueRef element_ptr;
                    LLVMTypeRef elem_ty;

                    if (node->type->ty == STRUCT) {
                        element_ptr = LLVMBuildStructGEP2(
                            builder, var_type, alloca_ptr, i, "init_sgep");
                        // Find member type
                        Member* m = node->type->members;
                        for (int j = 0; j < i && m; j++)
                            m = m->next;
                        elem_ty = to_llvm_type(m ? m->type : NULL);
                    } else {
                        LLVMValueRef indices[] = {
                            LLVMConstInt(LLVMInt32Type(), 0, 0),
                            LLVMConstInt(LLVMInt32Type(), i, 0)};
                        element_ptr =
                            LLVMBuildInBoundsGEP2(builder, var_type, alloca_ptr,
                                                  indices, 2, "init_agep");
                        elem_ty = to_llvm_type(node->type->ptr_to);
                    }

                    LLVMValueRef cast_val = cast_value(builder, val, elem_ty);
                    LLVMBuildStore(builder, cast_val, element_ptr);
                    cur = cur->next;
                    i++;
                }
            } else {
                LLVMValueRef val = codegen(ctx, node->init, builder,
                                           local_allocas, has_return, module);
                LLVMValueRef cast_val =
                    cast_value(builder, val, to_llvm_type(node->type));
                LLVMBuildStore(builder, cast_val, alloca_ptr);
            }
        }
        return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case ND_RETURN: {
        if (node->lhs) {
            LLVMValueRef ret_val = codegen(ctx, node->lhs, builder,
                                           local_allocas, has_return, module);
            // Verify return type matches function return type
            if (ctx->current_func_type) {
                LLVMTypeRef func_ret_ty = to_llvm_type(ctx->current_func_type);
                ret_val = cast_value(builder, ret_val, func_ret_ty);
            }
            LLVMBuildRet(builder, ret_val);
        } else {
            LLVMBuildRetVoid(builder);
        }
        *has_return = true;
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
    case ND_LOGAND: {
        // Short-circuiting &&: lhs ? rhs != 0 : 0
        LLVMValueRef func =
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
        LLVMBasicBlockRef rhs_bb = LLVMAppendBasicBlock(func, "logand_rhs");
        LLVMBasicBlockRef end_bb = LLVMAppendBasicBlock(func, "logand_end");

        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef lhs_cond = convert_to_bool(builder, lhs);
        LLVMBasicBlockRef lhs_bb = LLVMGetInsertBlock(builder);
        LLVMBuildCondBr(builder, lhs_cond, rhs_bb, end_bb);

        // Right hand side
        LLVMPositionBuilderAtEnd(builder, rhs_bb);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs_cond = convert_to_bool(builder, rhs);
        LLVMBuildBr(builder, end_bb);
        rhs_bb = LLVMGetInsertBlock(builder);

        // End block (PHI node)
        LLVMPositionBuilderAtEnd(builder, end_bb);
        LLVMValueRef phi = LLVMBuildPhi(builder, LLVMInt1Type(), "logical_res");
        LLVMValueRef incoming_values[] = {LLVMConstInt(LLVMInt1Type(), 0, 0),
                                          rhs_cond};
        LLVMBasicBlockRef incoming_blocks[] = {lhs_bb, rhs_bb};
        LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);
        return LLVMBuildZExt(builder, phi, LLVMInt32Type(), "logand_zext");
    }
    case ND_LOGOR: {
        // Short-circuiting ||: lhs ? 1 : (rhs != 0)
        LLVMValueRef func =
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
        LLVMBasicBlockRef rhs_bb = LLVMAppendBasicBlock(func, "logor_rhs");
        LLVMBasicBlockRef end_bb = LLVMAppendBasicBlock(func, "logor_end");

        LLVMValueRef lhs =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef lhs_cond = convert_to_bool(builder, lhs);
        LLVMBasicBlockRef lhs_bb = LLVMGetInsertBlock(builder);
        LLVMBuildCondBr(builder, lhs_cond, end_bb, rhs_bb);

        // Right hand side
        LLVMPositionBuilderAtEnd(builder, rhs_bb);
        LLVMValueRef rhs =
            codegen(ctx, node->rhs, builder, local_allocas, has_return, module);
        LLVMValueRef rhs_cond = convert_to_bool(builder, rhs);
        LLVMBuildBr(builder, end_bb);
        rhs_bb = LLVMGetInsertBlock(builder);

        // End block (PHI node)
        LLVMPositionBuilderAtEnd(builder, end_bb);
        LLVMValueRef phi = LLVMBuildPhi(builder, LLVMInt1Type(), "logical_res");
        LLVMValueRef incoming_values[] = {LLVMConstInt(LLVMInt1Type(), 1, 0),
                                          rhs_cond};
        LLVMBasicBlockRef incoming_blocks[] = {lhs_bb, rhs_bb};
        LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);
        return LLVMBuildZExt(builder, phi, LLVMInt32Type(), "logor_zext");
    }
    case ND_COND: {
        LLVMValueRef cond_val = codegen(ctx, node->cond, builder, local_allocas,
                                        has_return, module);
        LLVMValueRef cond_bool = convert_to_bool(builder, cond_val);

        LLVMValueRef func =
            LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
        LLVMBasicBlockRef then_bb = LLVMAppendBasicBlock(func, "ternary_then");
        LLVMBasicBlockRef else_bb = LLVMAppendBasicBlock(func, "ternary_else");
        LLVMBasicBlockRef end_bb = LLVMAppendBasicBlock(func, "ternary_end");

        LLVMBuildCondBr(builder, cond_bool, then_bb, else_bb);

        // Then branch
        LLVMPositionBuilderAtEnd(builder, then_bb);
        bool then_ret = false;
        LLVMValueRef then_val =
            codegen(ctx, node->lhs, builder, local_allocas, &then_ret, module);
        LLVMBasicBlockRef then_end = LLVMGetInsertBlock(builder);
        LLVMBuildBr(builder, end_bb);

        // Else branch
        LLVMPositionBuilderAtEnd(builder, else_bb);
        bool else_ret = false;
        LLVMValueRef else_val =
            codegen(ctx, node->rhs, builder, local_allocas, &else_ret, module);
        LLVMBasicBlockRef else_end = LLVMGetInsertBlock(builder);
        LLVMBuildBr(builder, end_bb);

        // End block (PHI node)
        LLVMPositionBuilderAtEnd(builder, end_bb);

        // Match types for PHI
        LLVMTypeRef then_ty = LLVMTypeOf(then_val);
        LLVMTypeRef else_ty = LLVMTypeOf(else_val);
        if (then_ty != else_ty) {
            else_val = cast_value(builder, else_val, then_ty);
        }

        LLVMValueRef phi = LLVMBuildPhi(builder, then_ty, "ternary_res");
        LLVMValueRef incoming_values[2] = {then_val, else_val};
        LLVMBasicBlockRef incoming_blocks[2] = {then_end, else_end};
        LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);

        return phi;
    }
    case ND_NOT: {
        LLVMValueRef val =
            codegen(ctx, node->lhs, builder, local_allocas, has_return, module);
        LLVMValueRef cond = convert_to_bool(builder, val);
        LLVMValueRef res = LLVMBuildNot(builder, cond, "not_bool");
        return LLVMBuildZExt(builder, res, LLVMInt32Type(), "not_zext");
    }
    case ND_INIT: { // New case for initialization
        // ND_INIT node represents an initialization expression.
        // The value to be initialized is in node->lhs.
        // This node itself doesn't produce a value, but performs a side effect
        // (store). The actual store operation would typically be handled by the
        // parent ND_DECL. For now, we just codegen the lhs to ensure it's
        // evaluated. If ND_INIT is meant to return the value, it should be
        // node->lhs. Assuming it's just to evaluate the expression for its side
        // effects or to get its value.
        return codegen(ctx, node->lhs, builder, local_allocas, has_return,
                       module);
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
