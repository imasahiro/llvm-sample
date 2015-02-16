#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Intrinsics.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include <iostream>
/*
 * float f1(float p) { return sin(p); }
 */
using namespace llvm;

int main(int argc, char **argv)
{
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    Module *m = new Module("test", Context);
    Type *floatTy = Type::getFloatTy(Context);

    std::vector<Type*> List;
    List.push_back(floatTy);
    FunctionType *fnTy = FunctionType::get(floatTy, List, false);
    Function *func = Function::Create(fnTy, GlobalValue::ExternalLinkage, "f1", m);
    Value *v = func->arg_begin();

    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);
    Function *fsin = Intrinsic::getDeclaration(m, Intrinsic::sin, List);
    v = builder->CreateCall(fsin, v);
    builder->CreateRet(v);
    ExecutionEngine *ee = EngineBuilder(m).setEngineKind(EngineKind::JIT).create();

    (*m).dump();

    void *f = ee->getPointerToFunction(func);
    std::cout << ((float (*)(float))f)(10.0) << std::endl;
    std::cout << sin(10.0) << std::endl;

    return 0;
}

