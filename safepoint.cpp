#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Metadata.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/Statistic.h>

using namespace llvm;

namespace {
// SafePoint pass
struct SafePoint : public FunctionPass {
	static char ID;
	SafePoint() : FunctionPass(ID) {}
	virtual bool runOnFunction(Function &F) {
		Module *m = F.getParent();
		LLVMContext &Context = m->getContext();
		NamedMDNode *NMD = m->getNamedMetadata("safepoint");
		MDNode *node = NMD->getOperand(0);
		unsigned KindID = Context.getMDKindID("safepoint");
		bool Changed = false;
		Type *IntTy = Type::getInt32Ty(Context);

		std::vector<BasicBlock *> BBs;
		for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {
			TerminatorInst *Inst = I->getTerminator();
			if (Inst->hasMetadata()) {
				MDNode *MD = Inst->getMetadata(KindID);
				if (MD == node) {
					BBs.push_back(I);
					Changed = true;
				}
			}
		}
		Value *arg_m;
		if (Changed) {
			Function::arg_iterator args = F.arg_begin();
			arg_m = ++args;arg_m->setName("m");
		}
		for (std::vector<BasicBlock* >::iterator I = BBs.begin(), E = BBs.end(); I != E ; ++I) {
			BasicBlock *BB = *I;
			TerminatorInst *Inst = BB->getTerminator();
			if (BranchInst *BI = dyn_cast<BranchInst>(Inst)) {
				fprintf(stderr, "succ=%d\n", BI->getNumSuccessors());
			} else if (ReturnInst *RI = dyn_cast<ReturnInst>(Inst)) {
				BasicBlock *bb0 = BasicBlock::Create(Context, "bb0", &F);
				BasicBlock *bb1 = BasicBlock::Create(Context, "bb1", &F);
				BasicBlock *bb2 = BB->splitBasicBlock(--(BB->end()), "bb2");
				IRBuilder<> *builder = new IRBuilder<>(bb0);
				BB->getTerminator()->setSuccessor(0, bb0);
				Value *v;
				v = builder->CreateICmpSLE(arg_m, ConstantInt::get(IntTy, 10));
				builder->CreateCondBr(v, bb1, bb2);
				builder->SetInsertPoint(bb1);
				std::vector<Type*> argsTy;
				FunctionType *FT = FunctionType::get(Type::getVoidTy(Context), argsTy, false);
				Function *F = cast<Function>(m->getOrInsertFunction("__f__", FT));
				builder->CreateCall(F);
				builder->CreateBr(bb2);
				builder->SetInsertPoint(bb2);
			}
		}
		errs() << "SafePoint" << '\n';
		return Changed;
	}
};
}

char SafePoint::ID = 0;
static RegisterPass<SafePoint> X("SafePoint", "SafePoint World Pass");

