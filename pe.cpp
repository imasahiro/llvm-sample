#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Transforms/Utils/Cloning.h>
using namespace llvm;

/*
 * float t1(float p) { return (p>0)?p:-p; }
 */
Function *createAbs(Module *m, LLVMContext &Context, Type *floatTy) {
    Type *ArgTypes[] = { floatTy };
    FunctionType *Ty = FunctionType::get(floatTy, ArgTypes, false);
    Function *func = Function::Create(Ty,
            GlobalValue::ExternalLinkage, "fabs", m);

    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);

    Value *arg0 = func->arg_begin();arg0->setName("arg0");
    Value *v    = builder->CreateFCmpOGE(arg0, ConstantFP::get(floatTy, 0.0));
    Value *v0   = builder->CreateFNeg(arg0);
    Value *ret  = builder->CreateSelect(v, arg0, v0);
    builder->CreateRet(ret);
    return func;
}

Function *cloning(Function *F, Constant *C)
{
    SmallVector<ReturnInst*, 8> Returns;
    ClonedCodeInfo CodeInfo;
    ValueToValueMapTy VMap;
    for (Function::const_arg_iterator I = F->arg_begin(),
            E = F->arg_end(); I != E; ++I) {
        VMap[I] = C;
    }
    Function *NewF = Function::Create(F->getFunctionType(),
            GlobalValue::ExternalLinkage, F->getName()+".pe",
            F->getParent());
    CloneAndPruneFunctionInto(NewF, F, VMap, false,
            Returns, ".pe", &CodeInfo, 0, 0);
    return NewF;
}

int main(int argc, char **argv)
{
    LLVMContext &Context = getGlobalContext();
    Module *m = new Module("test", Context);
    Type *floatTy = Type::getFloatTy(Context);
    Constant *C = ConstantFP::get(floatTy, 3.00);
    Function *F = createAbs(m, Context, floatTy);
    F = cloning(F, C);
    (*m).dump();
    return 0;
}

