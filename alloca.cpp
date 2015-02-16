#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Support/TargetSelect.h>
#include "llvm/Support/IRBuilder.h"
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>

using namespace llvm;
// g++ struct.cpp `llvm-config --libs --ldflags --cxxflags all` -O0 -g3  && ./a.out

struct x {
    short s1;
    short s2;
    void *ptr;
};

typedef short (*func_t) (struct x *);
int main(int argc, char **argv)
{
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    Module *m = new Module("test", Context);
    Type *longTy  = Type::getInt32Ty(Context);
    Type *shortTy = Type::getInt16Ty(Context);
    Type *voidPtr = PointerType::get(longTy, 0);

    StructType* structTy;
    std::vector<Type*>fields;
    fields.push_back(shortTy);  /* s1 */
    fields.push_back(shortTy);  /* s1 */
    fields.push_back(voidPtr); /* ptr */
    structTy = StructType::create(fields, "struct.x", /*isPacked=*/false);

    std::vector<Type*>args_type;
    /*
     * short t1(struct x *p) {
     *   struct x v = {10, 20, 30};
     *   return v.s1 + p->s1;
     * }
     */
    args_type.push_back(PointerType::get(structTy, 0));
    FunctionType *tyFunc = FunctionType::get(shortTy, args_type, false);
    Function *func = Function::Create(tyFunc, GlobalValue::ExternalLinkage, "t1", m);
    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);

    Value *v, *arg  = func->arg_begin();
    v = builder->CreateConstGEP2_32(arg, 0, 1, "gep");
    v = builder->CreateLoad(v, "load");
    Value *tmp = builder->CreateAlloca(structTy);
    Value *field;
    field = builder->CreateConstGEP2_32(tmp, 0, 0, "gep");
    builder->CreateStore(ConstantInt::get(shortTy, 10), field);
    field = builder->CreateConstGEP2_32(tmp, 0, 1, "gep");
    builder->CreateStore(ConstantInt::get(shortTy, 20), field);
    field = builder->CreateLoad(field);
    v = builder->CreateAdd(v, field);
    builder->CreateRet(v);
    ExecutionEngine *ee = EngineBuilder(m).setEngineKind(EngineKind::JIT).create();
    PassManager mpm;
    mpm.add(createIPSCCPPass());
    mpm.add(createFunctionInliningPass());
    mpm.add(createLICMPass());
    mpm.add(createGVNPass());
    mpm.add(createGlobalDCEPass());
    mpm.run(*m);

    (*m).dump();

    void *f = ee->getPointerToFunction(func);
    struct x o = {10, 20, (void*)0xdeadbeaf};
    fprintf(stderr, "%d\n", ((func_t)f)(&o));

    return 0;
}

