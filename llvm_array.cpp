#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetSelect.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Metadata.h"

using namespace llvm;

typedef int (*func_t) (int *, int);
int main(int argc, char **argv)
{
	InitializeNativeTarget();
	LLVMContext &Context = getGlobalContext();
	Module *m = new Module("test", Context);
	const Type *tyInt32 = Type::getInt32Ty(Context);
	PointerType *tyPtr = PointerType::get(tyInt32, 0);
	//const Type *tyFloat32 = Type::getFloatTy(Context);

	std::vector<const Type*>args_type;
	args_type.push_back(tyPtr);
	args_type.push_back(tyInt32);

	FunctionType *tyFunc = FunctionType::get(tyInt32, args_type, false);
	/* int array(int, int) */
	Function *array = Function::Create(tyFunc, 
			GlobalValue::ExternalLinkage, "array", m);
	Function::arg_iterator args = array->arg_begin();
	Value *arg_n = args++;
	arg_n->setName("n");
	Value *arg_m = args++;
	arg_m->setName("m");

	BasicBlock *bb = BasicBlock::Create(Context, "EntryBlock", array);
	IRBuilder<> *builder = new IRBuilder<>(bb);
	ExecutionEngine *ee = EngineBuilder(m).setEngineKind(EngineKind::JIT).create();

	Value* ptr = builder->CreateGEP(arg_n, arg_m, "");
	LoadInst* a = builder->CreateLoad(ptr, false, "");
	Instruction *inst = builder->CreateRet(a);

	//MDNode *n = MDNode::get(Context, inst);
	const char *Name  = "sample";
	const char *Name2 = "safepoint";

	Constant *Str = ConstantArray::get(Context, Name, true);
	GlobalVariable *GV = new GlobalVariable(*m, Str->getType(),
			true, GlobalValue::InternalLinkage, Str, "", 0, false);

	std::vector<Value *> V;
	V.push_back(GV);
	MDNode *node = MDNode::get(Context, V);
	NamedMDNode *NMD = m->getOrInsertNamedMetadata(Name);
	unsigned KindID = Context.getMDKindID(Name);
	NMD->addOperand(node);
	inst->setMetadata(KindID, node);
	(*m).dump();

	fprintf(stderr, "*****************\n");

	for (Function::iterator I = array->begin(), E = array->end(); I != E; ++I) {
		TerminatorInst *Inst = I->getTerminator();
		if (Inst->hasMetadata()) {
			MDNode *MD = Inst->getMetadata(KindID);
			if (MD == node) {
				MD->dump();
			}
		}
	}

	void *f = ee->getPointerToFunction(array);
	int iarray[] = {1, 2, 3, 4, 5};

	fprintf(stderr, "%d\n", ((func_t)f)(iarray, 3));

	return 0;
}

