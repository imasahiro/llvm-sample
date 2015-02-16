#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Linker.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/system_error.h>
#include <llvm/ADT/OwningPtr.h>
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

Module *LoadModule(const char *filename)
{
    LLVMContext &Context = getGlobalContext();
    std::string ErrMsg;
    OwningPtr<MemoryBuffer> Buffer;
    std::string fname(filename);
    if (error_code ec = MemoryBuffer::getFile(fname, Buffer)) {
        std::cout << "Could not open file" << ec.message() << std::endl;
    }
    Module *m = ParseBitcodeFile(Buffer.get(), Context, &ErrMsg);
    if (!m) {
        std::cout << "error" << ErrMsg << std::endl;
    }
    return m;
}

Function *CreateF(Module *m, Type *ctxTy, Type *sfpTy)
{
    LLVMContext &Context = getGlobalContext();
    Type *ArgsTy[] = {
        PointerType::get(ctxTy, 0),
        PointerType::get(sfpTy, 0),
        Type::getInt64Ty(Context)
    };

    FunctionType *fnTy = FunctionType::get(Type::getVoidTy(Context), ArgsTy, false);
    Function *F = cast<Function>(m->getFunction("Float_random"));
    return F;
}

int main(int argc, char **argv)
{
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    Module *m = new Module("test", Context);
    Module *TheM = LoadModule("struct.c.bc");

    std::string ErrMsg;
    if (Linker::LinkModules(m, TheM, Linker::DestroySource, &ErrMsg)) {
        std::cout << "error" << ErrMsg << std::endl;
        return 1;
    }

    StructType* ctxTy = StructType::create(Context, "struct.kcontext_t");
    StructType* sfpTy = StructType::create(Context, "struct.ksfp_t");
    Type *Int64Ty = Type::getInt64Ty(Context);
    Type *ArgsTy[] = {
        PointerType::get(ctxTy, 0),
        PointerType::get(sfpTy, 0),
        Int64Ty
    };
    Type *floatTy = Type::getDoubleTy(Context);
    FunctionType *fnTy = FunctionType::get(floatTy, ArgsTy, false);
    Function *Frand = CreateF(m, ctxTy, sfpTy);

    Function *F = Function::Create(fnTy, GlobalValue::ExternalLinkage, "test", m);
    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", F);
    IRBuilder<> *builder = new IRBuilder<>(bb);
    Function::arg_iterator I = F->arg_begin();
    Value *Ctx = I++;
    Value *Sfp = I++;
    Value *Rix = I;
    Value *Args[] = {
        Ctx, Sfp, Rix
    };
    Value *v = builder->CreateCall(Frand, Args);
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

    //{
    //    void *ptr = ee->getPointerToFunction(F);
    //    typedef struct ksfp_t {
    //        double v;
    //        void *p;
    //    } ksfp_t;
    //    typedef float (*F_t)(void *, ksfp_t *, long);
    //    ksfp_t sfp_[100] = {};
    //    F_t fptr = (F_t) ptr;
    //    std::cout << fptr(NULL, sfp_, 0) << std::endl;
    //    asm volatile("int3");
    //    std::cout << ((float (*)())ptr)() << std::endl;
    //}
    return 0;
}

