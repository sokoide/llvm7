/* Minimal llvm-c/Analysis.h for selfhost */
#include <llvm-c/Core.h>
typedef enum {
  LLVMAbortProcessAction,
  LLVMPrintMessageAction,
  LLVMReturnStatusAction
} LLVMVerifierFailureAction;
LLVMBool LLVMVerifyModule(LLVMModuleRef M, LLVMVerifierFailureAction Action, char** OutMessages);
