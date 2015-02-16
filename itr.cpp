#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
using namespace llvm;

Function *createFunc(Module *m) {
    /* void test() { return; } */
    Type *voidTy = Type::getVoidTy(m->getContext());
    std::vector<Type*> args;
    FunctionType *Ty = FunctionType::get(voidTy, args, false);
    Function *F = Function::Create(Ty, GlobalValue::ExternalLinkage, "test", m);
    BasicBlock *bb = BasicBlock::Create(m->getContext(), "EntryBlock", F);
    IRBuilder<> *builder = new IRBuilder<>(bb);
    builder->CreateRetVoid();
    return F;
}

int main(int argc, char **argv)
{
    LLVMContext &Context = getGlobalContext();
    Type   *intTy  = Type::getInt32Ty(Context);
    Module   *m    = new Module("test", Context);
    Function *F    = createFunc(m);
    Constant *Zero = ConstantInt::get(intTy, 0);
    BasicBlock *BB = F->begin();
    Instruction *I = --(BB->end());

    Type *types[]  = {intTy};
    Value *Idxs_[] = {Zero, Zero};

    std::vector<Type*>  fields(types, types+1);
    std::vector<Value*> Idxs(Idxs_, Idxs_+2);
    StructType* structTy = StructType::create(fields, "struct.x", false);

    AllocaInst *newinst    = new AllocaInst(structTy);
    GetElementPtrInst *gep = GetElementPtrInst::CreateInBounds(newinst, Idxs);
    StoreInst *store       = new StoreInst(ConstantInt::get(intTy, 10), gep);

    BB->getInstList().insert(I, newinst);
    BB->getInstList().insert(I, gep);
    BB->getInstList().insert(I, store);
    (*m).dump();
    return 0;
}

