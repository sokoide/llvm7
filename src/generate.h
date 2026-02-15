#ifndef __GENERATE_H__
#define __GENERATE_H__

#include "ast.h"

#include <llvm-c/Core.h>

extern void generate_code(Context* ctx);
extern LLVMModuleRef generate_module(Context* ctx);

#endif
