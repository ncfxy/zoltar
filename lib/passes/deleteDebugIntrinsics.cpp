#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Streams.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/ValueSymbolTable.h"
#include "llvm/Value.h"
#include "llvm/Support/CallSite.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/CFG.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Analysis/LoopInfo.h"
#include <time.h>

#include "registration.h"

using namespace llvm;

namespace {
  class DebugIntrinsicsDeleter : public ModulePass {
    bool runOnModule(Module &M);
  public:
    static char ID;
    DebugIntrinsicsDeleter() : ModulePass((intptr_t)&ID) {}
  };
}

char DebugIntrinsicsDeleter::ID = 0;
static RegisterPass<DebugIntrinsicsDeleter>
RPdeletedbgintrinsics("deleteDbgIntrinsics", "Delete debug intrinsic instructions.");

bool DebugIntrinsicsDeleter::runOnModule(Module &M) {
  cerr << "instrument: --- Delete Debug Intrinsics ---\n";
  int cnt = 0;
  for (Module::iterator F = M.begin(), ME = M.end(); F != ME; ++F) {
    for (Function::iterator BB = F->begin(), FE = F->end(); BB != FE; ++BB) {
      for (BasicBlock::iterator I = BB->begin(); I != BB->end(); I++) {
        if(isa<DbgInfoIntrinsic>(*I)) {
          I->eraseFromParent();
          cnt++;
        }
      }
    }
  }
  cerr << "instrument: deleted " << cnt << " debug statements\n";
  return true;
}

