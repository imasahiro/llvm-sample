#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>

#include <iostream>
using namespace llvm;

struct x { int l1; struct x *next; };

int main(int argc, char **argv)
{
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    Module *m = new Module("test", Context);
    Type *intTy  = Type::getInt64Ty(Context);

    StructType* structTy = StructType::create(Context, "struct.list");
    std::vector<Type*> fields;
    fields.push_back(intTy);
    fields.push_back(PointerType::get(structTy, 0));
    if (structTy->isOpaque()) {
        structTy->setBody(fields, false);
    }

    /*
     * int f1(struct x *p) { return p->next->l1; }
     */
    std::vector<Type*> args_type;
    args_type.push_back(PointerType::get(structTy, 0));

    FunctionType *fnTy = FunctionType::get(intTy, args_type, false);
    Function *func = Function::Create(fnTy,
            GlobalValue::ExternalLinkage, "f1", m);
    Value *v = func->arg_begin();
    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);
    v = builder->CreateStructGEP(v, 1);
    v = builder->CreateLoad(v, "load0");
    v = builder->CreateStructGEP(v, 0);
    v = builder->CreateLoad(v, "load1");
    builder->CreateRet(v);

    (*m).dump();
    {
        ExecutionEngine *ee = EngineBuilder(m). 
            setEngineKind(EngineKind::JIT).create();
        void *f = ee->getPointerToFunction(func);
        typedef int (*func_t) (struct x *);
        struct x o = {10, NULL}, v = {};
        v.next = &o;
        std::cout << ((func_t)f)(&v) << std::endl;
    }
    return 0;
}

