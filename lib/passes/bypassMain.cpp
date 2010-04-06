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
  class MainByPasser : public ModulePass {
    bool runOnModule(Module &M);
  public:
    static char ID;
    MainByPasser() : ModulePass((intptr_t)&ID) {}
  };
}

char MainByPasser::ID = 0;
static RegisterPass<MainByPasser>
RPbypassmain("bypassmain", "Bypass the main function for analysis purposes");

bool MainByPasser::runOnModule(Module &M) {

  cerr << "instrument: --- Bypass Main ---\n";

  // bypass the main function
  ValueSymbolTable& symtbl = M.getValueSymbolTable();
  Value* value = symtbl.lookup("main");
  if(value) {
    value->setName("_main_original");
    cerr << "instrument: bypassed main()\n";
  } else {
    cerr << "Warning: no main function in symbol table!\n";
  }

  // bypass the exit function
  value = symtbl.lookup("exit");
  if(value) {
    value->setName("_exithandler");
    cerr << "instrument: bypassed exit()\n";
  }

  // bypass the abort function
  value = symtbl.lookup("abort");
  if(value) {
    value->setName("_aborthandler");
    cerr << "instrument: bypassed abort()\n";
  }

  // add _registerAll() function if not yet added
  createRegisterAllFunction(M);

  return true;
}

