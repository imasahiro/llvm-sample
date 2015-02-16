/*
 * clang dump.cpp -emit-llvm `llvm-config --cxxflags` -c -o dump.bc
 * llvm-ld dump.bc -o dump
 * ./dump
 */
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/system_error.h>
#include <llvm/ADT/OwningPtr.h>
#include <iostream>
using namespace llvm;

int main(int argc, char **argv)
{
    LLVMContext &Context = getGlobalContext();
    std::string ErrMsg;
    OwningPtr<MemoryBuffer> Buffer;
    std::string fname(argv[1]);
    if (error_code ec = MemoryBuffer::getFile(fname+".bc", Buffer)) {
        std::cout << "Could not open file" << ec.message() << std::endl;
    }
    Module *m = ParseBitcodeFile(Buffer.get(), Context, &ErrMsg);
    if (!m) {
        std::cout << "error" << ErrMsg << std::endl;
        return 1;
    }
    (*m).dump();
    return 0;
}

