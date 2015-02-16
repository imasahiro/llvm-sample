#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/GlobalVariable.h>
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

struct x {
    int v;
};
struct x *global_value;
struct x *global_value2;

int main(int argc, char **argv)
{
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    Module *m = new Module("test", Context);
    Type *intTy  = Type::getInt32Ty(Context);

    std::vector<Type*> fields;
    fields.push_back(intTy);  /* v  */

    StructType* structTy = StructType::create(Context, fields, "struct.x", false);
    PointerType *ptrTy = PointerType::get(structTy, 0);

    std::vector<Type*>args_type;
    args_type.push_back(ptrTy);
    FunctionType *tyFunc = FunctionType::get(intTy, args_type, false);
    Function *func = Function::Create(tyFunc, GlobalValue::InternalLinkage, "test", m);
    BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", func);
    IRBuilder<> *builder = new IRBuilder<>(bb);
    GlobalVariable *g = new GlobalVariable(*m, ptrTy, false, GlobalValue::ExternalLinkage, 0, "gvalue");
    GlobalVariable *g1 = new GlobalVariable(*m, ptrTy, false, GlobalValue::ExternalLinkage, 0, "gvalue");

    //Value *v0 = builder->CreateStructGEP(func->arg_begin(), 0);
    //builder->CreateLoad(v0);
    Value *v1 = builder->CreateLoad(g);
    v1 = builder->CreateStructGEP(v1, 0);
    v1 = builder->CreateLoad(v1);
    builder->CreateRet(v1);
    //v0 = builder->CreateAdd(v0, v1);
    //builder->CreateRet(v0);
    ExecutionEngine *ee = EngineBuilder(m).setEngineKind(EngineKind::JIT).create();
    ee->addGlobalMapping(g, &global_value);
    ee->addGlobalMapping(g1, &global_value2);
    (*m).dump();

    void *ptr = ee->getPointerToFunction(func);
    struct x x = {10};
    struct x y = {20};
    global_value = &y;
    typedef int(*func_t)(struct x*);
    int p = ((func_t)ptr)(&x);
    fprintf(stderr, "%d\n", p);
    return 0;
}

