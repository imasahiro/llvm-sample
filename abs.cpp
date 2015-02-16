#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <iostream>
using namespace llvm;

Function *createAbs(Module *m, LLVMContext &Context, Type *floatTy) {
    std::vector<Type*>args_type;
    args_type.push_back(floatTy);
    FunctionType *tyFunc = FunctionType::get(floatTy, args_type, false);
    Function *func = Function::Create(tyFunc,
            GlobalValue::InternalLinkage, "fabs", m);
    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);
    Function::arg_iterator args = func->arg_begin();
    Value *arg0 = args++;arg0->setName("arg0");
    /*
     * float t1(float p) { return (p>0)?p:-p; }
     */
    Value *v    = builder->CreateFCmpOGE(arg0, ConstantFP::get(floatTy, 0.0));
    Value *v0   = builder->CreateFNeg(arg0);
    Value *ret  = builder->CreateSelect(v, arg0, v0);
    builder->CreateRet(ret);
    return func;
}

int main(int argc, char **argv)
{
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    Module *m = new Module("test", Context);
    Type *floatTy = Type::getFloatTy(Context);
    Function *f = createAbs(m, Context, floatTy);

    std::vector<Type*>args_type;
    args_type.push_back(floatTy);
    FunctionType *tyFunc = FunctionType::get(floatTy, args_type, false);
    Function *func = Function::Create(tyFunc, GlobalValue::ExternalLinkage, "test", m);
    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);
    Value *v = ConstantFP::get(floatTy, -10.0);
    v = builder->CreateCall(f, v);
    builder->CreateRet(v);
    std::cout << "before" << std::endl;
    (*m).dump();
    ExecutionEngine *ee = EngineBuilder(m).setEngineKind(EngineKind::JIT).create();
    PassManager mpm;
    mpm.add(createIPSCCPPass());
    mpm.add(createFunctionInliningPass());
    mpm.add(createLICMPass());
    mpm.add(createGVNPass());
    mpm.add(createGlobalDCEPass());
    mpm.run(*m);
    std::cout << std::endl << "before" << std::endl;
    (*m).dump();

    void *ptr = ee->getPointerToFunction(func);
    std::cout << ((float (*)(float))ptr)(-10.0) << std::endl;
    std::cout << fabs(-10.0) << std::endl;
    return 0;
}

