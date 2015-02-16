#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <iostream>
#include <assert.h>
using namespace llvm;
static const char* GetHostTriple() {
#ifdef LLVM_HOSTTRIPLE
  return LLVM_HOSTTRIPLE;
#else
  return LLVM_DEFAULT_TARGET_TRIPLE;
#endif
}

int main(int argc, char const* argv[])
{
    InitializeNativeTarget();
    std::string Error;
    Module *M = new Module("hi", getGlobalContext());
    const Target* T(TargetRegistry::lookupTarget(GetHostTriple(), Error));
    TargetOptions options;
    //options.NoFramePointerElim = true;
    if (DEBUG_MODE) {
        options.JITEmitDebugInfo = true;
    }
    TargetMachine* TM = T->createTargetMachine(GetHostTriple(), "", "", options);
    const TargetData *TD = TM->getTargetData();
    M->setDataLayout(TD->getStringRepresentation());
    M->setTargetTriple(TM->getTargetTriple());
    M->dump();
    return 0;
}
