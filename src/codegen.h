#ifndef __CODEGEN_H__
#define __CODEGEN_H__

#include "parse.h"

#include <llvm-c/Core.h>

extern void generate_code(Context* ctx);
extern LLVMModuleRef generate_module(Context* ctx);
extern int generate_code_to_file(Context* ctx, const char* filename);

#endif
