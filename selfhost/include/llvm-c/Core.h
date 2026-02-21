#ifndef LLVM_C_CORE_H
#define LLVM_C_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef unsigned char LLVMBool;

typedef struct LLVMOpaqueContext *LLVMContextRef;
typedef struct LLVMOpaqueModule *LLVMModuleRef;
typedef struct LLVMOpaqueType *LLVMTypeRef;
typedef struct LLVMOpaqueValue *LLVMValueRef;
typedef struct LLVMOpaqueBuilder *LLVMBuilderRef;
typedef struct LLVMOpaqueBasicBlock *LLVMBasicBlockRef;

typedef enum {
  LLVMVoidTypeKind = 0,
  LLVMHalfTypeKind = 1,
  LLVMFloatTypeKind = 2,
  LLVMDoubleTypeKind = 3,
  LLVMX86_FP80TypeKind = 4,
  LLVMFP128TypeKind = 5,
  LLVMPPC_FP128TypeKind = 6,
  LLVMLabelTypeKind = 7,
  LLVMIntegerTypeKind = 8,
  LLVMFunctionTypeKind = 9,
  LLVMStructTypeKind = 10,
  LLVMArrayTypeKind = 11,
  LLVMPointerTypeKind = 12,
  LLVMVectorTypeKind = 13,
  LLVMMetadataTypeKind = 14,
  LLVMTokenTypeKind = 16,
  LLVMScalableVectorTypeKind = 17,
  LLVMBFloatTypeKind = 18,
  LLVMX86_AMXTypeKind = 19,
  LLVMTargetExtTypeKind = 20
} LLVMTypeKind;

typedef enum {
  LLVMIntEQ = 32,
  LLVMIntNE,
  LLVMIntUGT,
  LLVMIntUGE,
  LLVMIntULT,
  LLVMIntULE,
  LLVMIntSGT,
  LLVMIntSGE,
  LLVMIntSLT,
  LLVMIntSLE
} LLVMIntPredicate;

typedef enum {
  LLVMExternalLinkage,
  LLVMAvailableExternallyLinkage,
  LLVMLinkOnceAnyLinkage,
  LLVMLinkOnceODRLinkage,
  LLVMLinkOnceODRAutoHideLinkage,
  LLVMWeakAnyLinkage,
  LLVMWeakODRLinkage,
  LLVMAppendingLinkage,
  LLVMInternalLinkage,
  LLVMPrivateLinkage,
  LLVMDLLImportLinkage,
  LLVMDLLExportLinkage,
  LLVMExternalWeakLinkage,
  LLVMGhostLinkage,
  LLVMCommonLinkage,
  LLVMLinkerPrivateLinkage,
  LLVMLinkerPrivateWeakLinkage
} LLVMLinkage;

typedef enum {
  LLVMDefaultVisibility,
  LLVMHiddenVisibility,
  LLVMProtectedVisibility
} LLVMVisibility;

typedef enum {
  LLVMNoUnnamedAddr,
  LLVMLocalUnnamedAddr,
  LLVMGlobalUnnamedAddr
} LLVMUnnamedAddr;

typedef enum {
  LLVMDefaultStorageClass = 0,
  LLVMDLLImportStorageClass = 1,
  LLVMDLLExportStorageClass = 2
} LLVMDLLStorageClass;

typedef enum {
  LLVMCCallConv = 0,
  LLVMFastCallConv = 8,
  LLVMColdCallConv = 9,
  LLVMGHCCallConv = 10,
  LLVMHiPECallConv = 11,
  LLVMAnyRegCallConv = 13
} LLVMCallConv;

LLVMModuleRef LLVMModuleCreateWithName(const char *ModuleID);
void LLVMDisposeModule(LLVMModuleRef M);
void LLVMDumpModule(LLVMModuleRef M);

LLVMBuilderRef LLVMCreateBuilder(void);
void LLVMDisposeBuilder(LLVMBuilderRef B);

LLVMBasicBlockRef LLVMAppendBasicBlock(LLVMValueRef Fn, const char *Name);
void LLVMPositionBuilderAtEnd(LLVMBuilderRef B, LLVMBasicBlockRef BB);

LLVMTypeRef LLVMInt1Type(void);
LLVMTypeRef LLVMInt8Type(void);
LLVMTypeRef LLVMInt32Type(void);
LLVMTypeRef LLVMInt64Type(void);
LLVMTypeRef LLVMVoidType(void);
LLVMTypeRef LLVMPointerType(LLVMTypeRef ElementType, unsigned AddrSpace);
LLVMTypeRef LLVMArrayType(LLVMTypeRef ElementType, unsigned ElementCount);
LLVMTypeRef LLVMFunctionType(LLVMTypeRef ReturnType, LLVMTypeRef *ParamTypes,
                            unsigned ParamCount, LLVMBool IsVarArg);
LLVMTypeRef LLVMStructCreateNamed(LLVMContextRef C, const char *Name);
void LLVMStructSetBody(LLVMTypeRef StructTy, LLVMTypeRef *ElementTypes,
                       unsigned ElementCount, LLVMBool Packed);

LLVMContextRef LLVMGetGlobalContext(void);

LLVMValueRef LLVMConstInt(LLVMTypeRef IntTy, unsigned long long N,
                          LLVMBool Signed);
LLVMValueRef LLVMConstNull(LLVMTypeRef Ty);
LLVMValueRef LLVMConstPointerNull(LLVMTypeRef Ty);
LLVMValueRef LLVMConstArray(LLVMTypeRef Ty, LLVMValueRef *Vals,
                            unsigned Count);
LLVMValueRef LLVMConstInBoundsGEP2(LLVMTypeRef Ty, LLVMValueRef Pointer,
                                   LLVMValueRef *Indices, unsigned NumIndices);
LLVMValueRef LLVMConstStringInContext(LLVMContextRef C, const char *Str,
                                      unsigned Length,
                                      LLVMBool DontNullTerminate);

LLVMValueRef LLVMGetNamedFunction(LLVMModuleRef M, const char *Name);
LLVMValueRef LLVMGlobalGetValueType(LLVMValueRef Val);
LLVMValueRef LLVMGetNamedGlobal(LLVMModuleRef M, const char *Name);
LLVMValueRef LLVMGetParam(LLVMValueRef Fn, unsigned Index);
void LLVMGetParamTypes(LLVMTypeRef FnTy, LLVMTypeRef *Dest);
LLVMTypeRef LLVMGetReturnType(LLVMTypeRef FnTy);
LLVMValueRef LLVMGetBasicBlockParent(LLVMBasicBlockRef BB);
LLVMValueRef LLVMGetBasicBlockTerminator(LLVMBasicBlockRef BB);
LLVMBasicBlockRef LLVMGetInsertBlock(LLVMBuilderRef B);
LLVMTypeRef LLVMTypeOf(LLVMValueRef Val);
LLVMTypeKind LLVMGetTypeKind(LLVMTypeRef Ty);
unsigned LLVMGetIntTypeWidth(LLVMTypeRef Ty);
LLVMBool LLVMIsFunctionVarArg(LLVMTypeRef FnTy);
unsigned LLVMCountParamTypes(LLVMTypeRef FnTy);

LLVMValueRef LLVMAddFunction(LLVMModuleRef M, const char *Name,
                             LLVMTypeRef FunctionTy);
LLVMValueRef LLVMAddGlobal(LLVMModuleRef M, LLVMTypeRef Ty, const char *Name);
LLVMValueRef LLVMBasicBlockAsValue(LLVMBasicBlockRef Block);
void LLVMAddIncoming(LLVMValueRef PhiNode, LLVMValueRef *IncomingValues,
                     LLVMBasicBlockRef *IncomingBlocks, unsigned Count);
void LLVMAddCase(LLVMValueRef Switch, LLVMValueRef OnVal,
                 LLVMBasicBlockRef Dest);

LLVMValueRef LLVMBuildAdd(LLVMBuilderRef B, LLVMValueRef LHS, LLVMValueRef RHS,
                          const char *Name);
LLVMValueRef LLVMBuildAlloca(LLVMBuilderRef B, LLVMTypeRef Ty,
                             const char *Name);
LLVMValueRef LLVMBuildBitCast(LLVMBuilderRef B, LLVMValueRef Val,
                              LLVMTypeRef DestTy, const char *Name);
LLVMValueRef LLVMBuildBr(LLVMBuilderRef B, LLVMBasicBlockRef Dest);
LLVMValueRef LLVMBuildCall2(LLVMBuilderRef B, LLVMTypeRef Ty, LLVMValueRef Fn,
                            LLVMValueRef *Args, unsigned NumArgs,
                            const char *Name);
LLVMValueRef LLVMBuildCondBr(LLVMBuilderRef B, LLVMValueRef If,
                             LLVMBasicBlockRef Then, LLVMBasicBlockRef Else);
LLVMValueRef LLVMBuildICmp(LLVMBuilderRef B, LLVMIntPredicate Predicate,
                           LLVMValueRef LHS, LLVMValueRef RHS,
                           const char *Name);
LLVMValueRef LLVMBuildInBoundsGEP2(LLVMBuilderRef B, LLVMTypeRef Ty,
                                   LLVMValueRef Pointer, LLVMValueRef *Indices,
                                   unsigned NumIndices, const char *Name);
LLVMValueRef LLVMBuildIntToPtr(LLVMBuilderRef B, LLVMValueRef Val,
                               LLVMTypeRef DestTy, const char *Name);
LLVMValueRef LLVMBuildIsNotNull(LLVMBuilderRef B, LLVMValueRef Val,
                                const char *Name);
LLVMValueRef LLVMBuildLoad2(LLVMBuilderRef B, LLVMTypeRef Ty,
                            LLVMValueRef PointerVal, const char *Name);
LLVMValueRef LLVMBuildMul(LLVMBuilderRef B, LLVMValueRef LHS, LLVMValueRef RHS,
                          const char *Name);
LLVMValueRef LLVMBuildNeg(LLVMBuilderRef B, LLVMValueRef V, const char *Name);
LLVMValueRef LLVMBuildNot(LLVMBuilderRef B, LLVMValueRef V, const char *Name);
LLVMValueRef LLVMBuildPhi(LLVMBuilderRef B, LLVMTypeRef Ty, const char *Name);
LLVMValueRef LLVMBuildPtrDiff2(LLVMBuilderRef B, LLVMTypeRef Ty,
                               LLVMValueRef LHS, LLVMValueRef RHS,
                               const char *Name);
LLVMValueRef LLVMBuildPtrToInt(LLVMBuilderRef B, LLVMValueRef Val,
                               LLVMTypeRef DestTy, const char *Name);
LLVMValueRef LLVMBuildRet(LLVMBuilderRef B, LLVMValueRef Val);
LLVMValueRef LLVMBuildRetVoid(LLVMBuilderRef B);
LLVMValueRef LLVMBuildSDiv(LLVMBuilderRef B, LLVMValueRef LHS,
                           LLVMValueRef RHS, const char *Name);
LLVMValueRef LLVMBuildSExt(LLVMBuilderRef B, LLVMValueRef Val,
                           LLVMTypeRef DestTy, const char *Name);
LLVMValueRef LLVMBuildSRem(LLVMBuilderRef B, LLVMValueRef LHS,
                           LLVMValueRef RHS, const char *Name);
LLVMValueRef LLVMBuildStore(LLVMBuilderRef B, LLVMValueRef Val,
                            LLVMValueRef Ptr);
LLVMValueRef LLVMBuildStructGEP2(LLVMBuilderRef B, LLVMTypeRef Ty,
                                 LLVMValueRef PointerVal, unsigned FieldNo,
                                 const char *Name);
LLVMValueRef LLVMBuildSub(LLVMBuilderRef B, LLVMValueRef LHS, LLVMValueRef RHS,
                          const char *Name);
LLVMValueRef LLVMBuildSwitch(LLVMBuilderRef B, LLVMValueRef V,
                             LLVMBasicBlockRef Else, unsigned NumCases);
LLVMValueRef LLVMBuildTrunc(LLVMBuilderRef B, LLVMValueRef Val,
                            LLVMTypeRef DestTy, const char *Name);
LLVMValueRef LLVMBuildZExt(LLVMBuilderRef B, LLVMValueRef Val,
                           LLVMTypeRef DestTy, const char *Name);

LLVMValueRef LLVMConstArray(LLVMTypeRef Ty, LLVMValueRef *Vals,
                            unsigned Count);
LLVMValueRef LLVMConstPointerNull(LLVMTypeRef Ty);
LLVMValueRef LLVMConstNull(LLVMTypeRef Ty);
LLVMValueRef LLVMConstStringInContext(LLVMContextRef C, const char *Str,
                                      unsigned Length,
                                      LLVMBool DontNullTerminate);

unsigned LLVMCountParamTypes(LLVMTypeRef FnTy);
void LLVMDisposeMessage(char *Message);

LLVMValueRef LLVMAddFunction(LLVMModuleRef M, const char *Name,
                             LLVMTypeRef FunctionTy);
LLVMValueRef LLVMAddGlobal(LLVMModuleRef M, LLVMTypeRef Ty, const char *Name);

LLVMValueRef LLVMBasicBlockAsValue(LLVMBasicBlockRef Block);

void LLVMAddIncoming(LLVMValueRef PhiNode, LLVMValueRef *IncomingValues,
                     LLVMBasicBlockRef *IncomingBlocks, unsigned Count);
void LLVMAddCase(LLVMValueRef Switch, LLVMValueRef OnVal,
                 LLVMBasicBlockRef Dest);

int LLVMPrintModuleToFile(LLVMModuleRef M, const char *Filename,
                          char **ErrorMessage);
void LLVMSetGlobalConstant(LLVMValueRef Global, LLVMBool Value);
void LLVMSetInitializer(LLVMValueRef Global, LLVMValueRef ConstantVal);
void LLVMSetLinkage(LLVMValueRef Global, LLVMLinkage Linkage);
void LLVMSetOperand(LLVMValueRef Val, unsigned Index, LLVMValueRef Operand);

#ifdef __cplusplus
}
#endif

#endif /* LLVM_C_CORE_H */
