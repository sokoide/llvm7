#ifndef __GENERATE_H__
#define __GENERATE_H__

#include <llvm-c/Core.h>

typedef struct {
    int value;
} ReturnExpr;

extern void generate_code(ReturnExpr* ast);
LLVMModuleRef generate_code_to_module(ReturnExpr* ast);

#endif