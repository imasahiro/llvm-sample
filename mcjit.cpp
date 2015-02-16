#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/JITMemoryManager.h>
#include <llvm/DataLayout.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Intrinsics.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/DynamicLibrary.h>
#include <string>
#include <iostream>

using namespace llvm;
extern "C" {

float run(float x) {
    return 100.0;
}

int main(int argc, char const* argv[])
{
InitializeNativeTarget();
InitializeNativeTargetAsmPrinter();
Module *Mod = new Module("LLVM", getGlobalContext());
Mod->setTargetTriple(LLVM_HOSTTRIPLE);
std::string Error;
JITMemoryManager *JMM = JITMemoryManager::CreateDefaultMemManager();
ExecutionEngine *EE = EngineBuilder(Mod)
    .setEngineKind(EngineKind::JIT)
    .setUseMCJIT(true)
    .setJITMemoryManager(JMM)
    .setErrorStr(&Error).create();

    if(Error != "") {
        std::cout << Error << std::endl;
        exit(EXIT_FAILURE);
    }

    /*XXX LLVM need to dlopen for */
    if(!sys::DynamicLibrary::LoadLibraryPermanently(NULL, &Error)) {
        std::cout << Error << std::endl;
    }

    LLVMContext &Context = getGlobalContext();
    Type *IntTy = Type::getInt32Ty(Context);

    std::vector<Type*> List;
    List.push_back(IntTy);
    FunctionType *FnTy = FunctionType::get(IntTy, List, false);
    Function *func = Function::Create(FnTy, GlobalValue::ExternalLinkage, "f1", Mod);
    Value *v = func->arg_begin();

    Function *Run = Function::Create(FnTy,GlobalValue::ExternalLinkage, "run", Mod);
    EE->addGlobalMapping(G, ptr);

    std::cout <<  Mod->getNamedGlobal("run") << std::endl;
    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);
    v = builder->CreateCall(G, v);
    builder->CreateRet(v);
    (*Mod).dump();

    void *f = EE->getPointerToFunction(func);
    union {
        void *ptr;
        float (*F)(float);
    } V; V.ptr = f;
    std::cout << V.F(10.0) << std::endl;
    std::cout << sin(10.0) << std::endl;

    return 0;
}

} /* extern "C" */
