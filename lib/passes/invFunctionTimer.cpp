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
#include "llvm/Support/IRBuilder.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Analysis/LoopInfo.h"
#include <time.h>

#include "indexManager.h"
#include "contextManager.h"
#include "registration.h"

using namespace llvm;

namespace {
  class InvFunctionTimerInstrumenter : public ModulePass {
    int numFunctionsInstrumented;
    bool runOnModule(Module &M);
  public:
    static char ID;
    InvFunctionTimerInstrumenter() : ModulePass((intptr_t)&ID) {numFunctionsInstrumented = 0;}
  };
}


char InvFunctionTimerInstrumenter::ID = 0;
static RegisterPass<InvFunctionTimerInstrumenter>
RPinvfunctiontimer("invfunctiontimer", "Insert instrumentation for function timer invariant");

bool InvFunctionTimerInstrumenter::runOnModule(Module &M) {
  Function *Main = M.getFunction("main");
  LLVMContext &C = M.getContext();
  
  cerr << "instrument: --- Function Timer Invariant ---\n";
 
  if (Main == 0) {
    cerr << "WARNING: cannot insert function instrumentation into a module"
         << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  // Add library function prototype
  Constant *NotifyFn = M.getOrInsertFunction("_handleInvariantChangeInt", 
                              Type::getVoidTy(C),   // returns void
                              Type::getInt32Ty(C),  // InvariantTypeIndex
                              Type::getInt32Ty(C),  // FunctionIndex
                              Type::getInt32Ty(C),  // Value
                              NULL);

  unsigned invariantTypeIndex = IndexManager::getInvariantTypeIndex();
  unsigned nInvariants = 0;
  
  // Loop through all functions within module
  for (Module::iterator F = M.begin(), ME = M.end(); F != ME; ++F) {

    // skip function declarations
    if(F->isDeclaration()) 
      continue;

    // skip the _registerAll function
    if(F->getName()=="_registerAll")
      continue;

    // locate insertion position at beginning of function
    BasicBlock *bb = F->begin();
    BasicBlock::iterator InsertPos = bb->begin();
    while (isa<AllocaInst>(InsertPos)) ++InsertPos;

    // add call to lib function for notifying function entrance
    std::vector<Value*> Args(3);
    Args[0] = ConstantInt::get(Type::getInt32Ty(C), invariantTypeIndex);
    Args[1] = ConstantInt::get(Type::getInt32Ty(C), nInvariants);
    Args[2] = ConstantInt::get(Type::getInt32Ty(C), 0);
         
    CallInst::Create(NotifyFn, Args.begin(), Args.end(), "", InsertPos);

    // try to get context information of loop
    std::string dir="-", file="-", name="-";
    int line = 0;
    while (!isa<DbgStopPointInst>(InsertPos)) ++InsertPos;
    if(isa<DbgStopPointInst>(*InsertPos)) {
      DbgStopPointInst &DSPI = cast<DbgStopPointInst>(*InsertPos);
      
      llvm::GetConstantStringInfo(DSPI.getDirectory(), dir);
      llvm::GetConstantStringInfo(DSPI.getFileName(), file);
      line = DSPI.getLine();
    }
    name = F->getName();

    // add source context of this invariant to context file
    ContextManager::addInvariantTypeContext(
      invariantTypeIndex,     // invariantTypeIndex
      nInvariants,            // invariantIndex
      dir,                    // path
      file,                   // file
      line,                   // line
      name);                  // name

    // find all function exit points
    int nReturns = 0;
    for(Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {
      for(InsertPos = B->begin(); InsertPos != B->end(); ++InsertPos) {
        if(isa<ReturnInst>(InsertPos)) {
          nReturns++;
          // add cal to lib function for notifying function exit
          std::vector<Value*> Args(3);
          Args[0] = ConstantInt::get(Type::getInt32Ty(C), invariantTypeIndex);
          Args[1] = ConstantInt::get(Type::getInt32Ty(C), nInvariants);
          Args[2] = ConstantInt::get(Type::getInt32Ty(C), -1);

          CallInst::Create(NotifyFn, Args.begin(), Args.end(), "", InsertPos);
        }
      }
    }

    nInvariants++;
  }

  // add the registration of the instrumented invariants in the _registerAll() function
  addInvariantTypeRegistration(M, invariantTypeIndex, nInvariants, "Function Timer", 1);

  llvm::cerr << "instrument: " << nInvariants << " functions instrumented\n";

  // notify change of program 
  return true;
}






