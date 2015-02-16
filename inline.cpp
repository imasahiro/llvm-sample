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
#include <llvm/Transforms/Utils/Cloning.h>
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

Function *Inliner(Module *m, Function *F, Type *floatTy, Constant *c)
{
    ValueToValueMapTy VMap;
    std::vector<Type*>args_type;
    args_type.push_back(floatTy);
    FunctionType *Ty = FunctionType::get(floatTy, args_type, false);
    Function *newF   = Function::Create(Ty,
            GlobalValue::InternalLinkage, "fabs_1", m);

    newF->setLinkage(GlobalValue::InternalLinkage);
    for (Function::arg_iterator I = F->arg_begin(),
            E = F->arg_end(); I != E; ++I) {
        Value *val = c;
        VMap[I] = val;
    }
    SmallVector<ReturnInst*, 8> Returns;
    ClonedCodeInfo CodeInfo;
    CloneAndPruneFunctionInto(newF, F, VMap, false, Returns, ".",
            &CodeInfo, 0);
    return newF;
}

int main(int argc, char **argv)
{
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    Module *m = new Module("test", Context);
    Type *floatTy = Type::getFloatTy(Context);
    Function *Fabs = createAbs(m, Context, floatTy);

    std::vector<Type*>args_type;
    args_type.push_back(floatTy);
    FunctionType *Ty = FunctionType::get(floatTy, args_type, false);
    Function *func = Function::Create(Ty, GlobalValue::ExternalLinkage, "test", m);
    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);
    Value *v = ConstantFP::get(floatTy, -10.0);
    if (Constant *c = dyn_cast<Constant>(v)) {
        Fabs = Inliner(m, Fabs, floatTy, c);
    }
    v = builder->CreateCall(Fabs, v);
    builder->CreateRet(v);
    ExecutionEngine *ee = EngineBuilder(m).setEngineKind(EngineKind::JIT).create();
    PassManager mpm;
    mpm.add(createFunctionInliningPass(225));
    mpm.add(createGlobalDCEPass());
    mpm.run(*m);

    (*m).dump();
    void *ptr = ee->getPointerToFunction(func);
    std::cout << ((float (*)(float))ptr)(-10.0) << std::endl;
    std::cout << fabs(-10.0) << std::endl;
    return 0;
}

