#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Constants.h>
#include <llvm/GlobalVariable.h>
#include <llvm/Function.h>
#include <llvm/BasicBlock.h>

#include "llvm/Instructions.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/TargetSelect.h"

#include <iostream>
using namespace llvm;

struct x {
    long l1;
    void *ptr;
    short s1;
    float f1;
};

struct y {
    struct x *x;
};

typedef short (*func_t) (struct y *);
int main(int argc, char **argv)
{
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    Module *m = new Module("test", Context);
    Type *floatTy = Type::getFloatTy(Context);
    Type *longTy  = Type::getInt32Ty(Context);
    Type *shortTy = Type::getInt16Ty(Context);
    Type *voidPtr = PointerType::get(longTy, 0);

    StructType* structTy;

    std::vector<Type*> fields;
    fields.push_back(longTy);  /* l1  */
    fields.push_back(voidPtr); /* ptr */
    fields.push_back(shortTy);  /* s1 */
    fields.push_back(floatTy); /* f1 */
    structTy = StructType::create(Context, fields, "struct.x", false);
    std::vector<Type*> fields2;
    fields2.push_back(PointerType::get(structTy, 0));
    StructType *structYTy = StructType::create(Context, fields2, "struct.y", false);

    std::vector<Type*>args_type;
    /*
     * short t1(struct y *p) { return p->x->s1; }
     */
    args_type.push_back(PointerType::get(structYTy, 0));
    FunctionType *tyFunc = FunctionType::get(longTy, args_type, false);
    Function *func = Function::Create(tyFunc, GlobalValue::ExternalLinkage, "t1", m);
    Function::arg_iterator args = func->arg_begin();
    Value *v, *arg = args++;

    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);

    {
        std::vector<Value*> List;
        v = builder->CreateStructGEP(arg, 0);
        v = builder->CreateLoad(v, "load0");
        v = builder->CreateStructGEP(v, 2);
        v = builder->CreateLoad(v, "load1");
        builder->CreateRet(v);
    }
    ExecutionEngine *ee = EngineBuilder(m).setEngineKind(EngineKind::JIT).create();

    (*m).dump();

    {
        void *f = ee->getPointerToFunction(func);
        struct x o = {10, (void*)0xdeadbeaf, 20, 30.0};
        struct y v;
        v.x = &o;
        fprintf(stderr, "%d\n", ((func_t)f)(&v));
    }

    return 0;
}

