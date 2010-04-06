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

#include "indexManager.h"
#include "contextManager.h"
#include "registration.h"

using namespace llvm;

namespace {
  class InvLoopCountInstrumenter : public ModulePass {
    bool runOnModule(Module &M);
    void getAnalysisUsage(AnalysisUsage &AU) const;
    void instrumentLoop(Loop *loop, Constant *loopInitFn, Constant *loopIncrFn, unsigned int invariantTypeIndex, unsigned int *loopIndex);
  public:
    static char ID;
    InvLoopCountInstrumenter() : ModulePass((intptr_t)&ID) {}
  };
}

char InvLoopCountInstrumenter::ID = 0;
static RegisterPass<InvLoopCountInstrumenter>
RPinstrloops("invloopcount", "Instrument loops for invariant analysis");

void InvLoopCountInstrumenter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<LoopInfo>();
}

bool InvLoopCountInstrumenter::runOnModule(Module &M) {

  cerr << "instrument: --- Loop Count Invariant ---\n";

  Function *Main = M.getFunction("main");
  LLVMContext &C = M.getContext();
  
  if (Main == 0) {
    cerr << "WARNING: cannot insert loop count instrumentation into a module"
         << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  // Add library function prototypes
  Constant *loopInitFn = M.getOrInsertFunction("_handleInvariantChangeUInt", 
                              Type::getVoidTy(C),   // returns void
                              Type::getInt32Ty(C),  // InvariantTypeIndex
                              Type::getInt32Ty(C),  // LoopIndex
                              Type::getInt32Ty(C),  // Value
                              NULL);
  Constant *loopIncrFn = M.getOrInsertFunction("_handleInvariantIncrement", 
                              Type::getVoidTy(C),   // returns void
                              Type::getInt32Ty(C),  // InvariantTypeIndex
                              Type::getInt32Ty(C),  // LoopIndex
                              NULL);

  unsigned int invariantTypeIndex = IndexManager::getInvariantTypeIndex();
  unsigned int loopIndex = 0;
  
  // Loop through all functions within module
  for (Module::iterator F = M.begin(), ME = M.end(); F != ME; ++F) {

    // skip function declarations
    if(F->isDeclaration()) 
      continue;

    // skip the _registerAll function
    if(F->getName()=="_registerAll")
      continue;

    // get loop info analysis from LLVM
    LoopInfo &LI = getAnalysis<LoopInfo>(*F);

    // process all loops in function
    for (LoopInfo::iterator L = LI.begin(), LE = LI.end(); L != LE; ++L) {
      Loop *loop = *L;

      instrumentLoop(loop, loopInitFn, loopIncrFn, invariantTypeIndex, &loopIndex);
    }
  }

  // add the registration of the instrumented invariants in the _registerAll() function
  addInvariantTypeRegistration(M, invariantTypeIndex, loopIndex, "Loop Counter", 0);

  llvm::cerr << "instrument: " << loopIndex << " loops  instrumented\n";

  // notify change of program 
  return true;
}

void InvLoopCountInstrumenter::instrumentLoop(Loop *loop, Constant *loopInitFn, Constant *loopIncrFn, unsigned int invariantTypeIndex, unsigned int *loopIndex) {
  if(loop->getLoopPreheader() && loop->getHeader()) {
    
    // init counter in header
    BasicBlock::iterator initPos = loop->getLoopPreheader()->begin();
    while (isa<AllocaInst>(initPos)) ++initPos;
    std::vector<Value*> Args(3);
    Args[0] = ConstantInt::get(Type::getInt32Ty(loop->getLoopPreheader()->getContext()), invariantTypeIndex);
    Args[1] = ConstantInt::get(Type::getInt32Ty(loop->getLoopPreheader()->getContext()), *loopIndex);
    Args[2] = ConstantInt::get(Type::getInt32Ty(loop->getLoopPreheader()->getContext()), 0);
    CallInst::Create(loopInitFn, Args.begin(), Args.end(), "", initPos);

    // increment counter in first loop block
    BasicBlock *header = loop->getHeader();
    BasicBlock::iterator incrPos = header->begin();
    while (isa<AllocaInst>(incrPos)) ++incrPos;
    std::vector<Value*> Args2(2);
    Args2[0] = ConstantInt::get(Type::getInt32Ty(header->getContext()), invariantTypeIndex);
    Args2[1] = ConstantInt::get(Type::getInt32Ty(header->getContext()), *loopIndex);
    CallInst::Create(loopIncrFn, Args2.begin(), Args2.end(), "", incrPos);

    (*loopIndex)++;

    // try to get context information of loop
    std::string dir="-", file="-", name="-";
    int line = 0;
    while (!isa<DbgStopPointInst>(incrPos)) ++incrPos;
    if(isa<DbgStopPointInst>(*incrPos)) {
      DbgStopPointInst &DSPI = cast<DbgStopPointInst>(*incrPos);
      
      llvm::GetConstantStringInfo(DSPI.getDirectory(), dir);
      llvm::GetConstantStringInfo(DSPI.getFileName(), file);
      line = DSPI.getLine();
    }

    // add source context of this invariant to context file
    ContextManager::addInvariantTypeContext(
      invariantTypeIndex,     // invariantTypeIndex
      (*loopIndex)-1,         // invariantIndex
      dir,                    // path
      file,                   // file
      line,                   // line
      name);                  // name
  }

  // process loops within this loop
  for (Loop::iterator L = loop->begin(), LE = loop->end(); L != LE; ++L) {
    instrumentLoop(*L, loopInitFn, loopIncrFn, invariantTypeIndex, loopIndex);
  }
}


