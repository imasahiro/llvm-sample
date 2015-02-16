
#include <llvm/CodeGen/GCStrategy.h>
#include <llvm/CodeGen/GCMetadata.h>
#include <llvm/Support/Compiler.h>

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Constants.h>
#include <llvm/Instructions.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/DomPrinter.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/Analysis/RegionPrinter.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/Lint.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Metadata.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <iostream>

using namespace llvm;

class MyGCStrategy : public GCStrategy {
public:
	MyGCStrategy() {
		NeededSafePoints = 1 << GC::PostCall;
		//NeededSafePoints = 1 << GC::Loop
		//	| 1 << GC::Return
		//	| 1 << GC::PreCall
		//	| 1 << GC::PostCall;
		UsesMetadata = true;
		InitRoots = false;  // LLVM crashes with this on due to bitcasts.
	}
};

static GCRegistry::Add<MyGCStrategy> MyGCStrategyRegistration("MyGC", "My-GC");

typedef int (*func_t) (int *, int);
void opt(Module *m)
{
	PassManager mpm;
	mpm.add(createTypeBasedAliasAnalysisPass());
	mpm.add(createBasicAliasAnalysisPass());
	mpm.add(createSimplifyLibCallsPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createScalarReplAggregatesPass());
	mpm.add(createEarlyCSEPass());
	mpm.add(createLowerExpectIntrinsicPass());
	mpm.add(createIPSCCPPass());
	mpm.add(createGlobalOptimizerPass());
	mpm.add(createConstantMergePass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createGlobalOptimizerPass());
	mpm.add(createGlobalDCEPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createJumpThreadingPass());
	mpm.add(createScalarReplAggregatesPass());
	mpm.add(createFunctionAttrsPass());
	mpm.add(createGlobalsModRefPass());
	mpm.add(createLICMPass());
	mpm.add(createGVNPass());
	mpm.add(createMemCpyOptPass());
	mpm.add(createDeadStoreEliminationPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createJumpThreadingPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createGlobalDCEPass());
	mpm.run(*m);
}
extern "C" void __f__() {
	fprintf(stderr, "hi\n");
}
int main(int argc, char **argv)
{
	InitializeNativeTarget();
	LLVMContext &Context = getGlobalContext();
	Module *m = new Module("test", Context);

	Type *IntTy = Type::getInt32Ty(Context);
	PointerType *tyPtr = PointerType::get(IntTy, 0);

	std::vector<Type*>args_type;
	args_type.push_back(tyPtr);
	args_type.push_back(IntTy);

	FunctionType *tyFunc = FunctionType::get(IntTy, args_type, false);
	/* int F(int, int) */
	Function *F = Function::Create(tyFunc, GlobalValue::ExternalLinkage, "F", m);
	F->setGC("MyGC");

	Function::arg_iterator args = F->arg_begin();
	Value *arg_n = args++;arg_n->setName("n");
	Value *arg_m = args++;arg_m->setName("m");

	BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", F);
	IRBuilder<> *builder = new IRBuilder<>(bb);
	ExecutionEngine *ee = EngineBuilder(m).setEngineKind(EngineKind::JIT).create();

	Value* ptr = builder->CreateGEP(arg_n, arg_m, "");
	LoadInst* a = builder->CreateLoad(ptr, false, "");
	builder->CreateRet(a);

	opt(m);

	(*m).dump();
	void *f = ee->getPointerToFunction(F);
	int iarray[] = {1, 2, 3, 4, 5};

	fprintf(stderr, "%d\n", ((func_t)f)(iarray, 3));

	return 0;
}
