#ifndef __GENERATE_H__
#define __GENERATE_H__

#include <llvm-c/Core.h>

typedef struct {
    int value;
} ReturnExpr;

extern void generate_code(ReturnExpr* ast);
extern LLVMModuleRef generate_module(ReturnExpr* ast);

#endif