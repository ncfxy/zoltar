#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
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
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "Operator.h"

#include <list>
#include <map>


#include "MutatorPass.h"

using namespace llvm;


namespace {

  class Mutator : public ModulePass {
	bool runOnModule(Module &M);
  public:
    static char ID;
    Mutator() : ModulePass((intptr_t)&ID) {}
  };
	
}

static RegisterPass<Mutator>
RPmutarith("mutation", "Perform mutations");

static cl::list<unsigned>
MutationIDS("mutation-ids", cl::NotHidden, cl::CommaSeparated,
	 cl::desc("Control which mutation to apply (as obtained by -mutation-list)"));

char Mutator::ID = 0;

bool Mutator::runOnModule(Module &M) {
	
  unsigned siteId = 0;
  
  OperatorManager   *OMgr = OperatorManager::getInstance();
  OperatorInfoList  oplst;

  // Loop through all functions within module
  for (Module::iterator F = M.begin(), ME = M.end(); F != ME; ++F) {	
    // Loop through all basic blocks within function
    for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {
      // Loop through all instructions within basic block
      for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE; I++) {
        // Consider only mutable instructions
        OMgr->getCompatibleOperators(I, oplst);
        
        bool mutated = false;
        for (OperatorInfoList::iterator opi = oplst.begin(); opi != oplst.end(); opi++) {
          cl::list<unsigned>::iterator sid = find (MutationIDS.begin(), MutationIDS.end(), siteId++);
          if (sid != MutationIDS.end()) {
            // One of the specified mutations was found
            if (!mutated) {
              MutationOperator *op = (*opi)->build();
              Value *newv = op->apply(I);
              //cerr << *I << " --> " << *newv << "\n";
              
              if (newv != NULL) {
                  ReplaceInstWithValue(B->getInstList(), I, newv);
              } else {
                  
              }
              mutated = true;
            } else {
              throw std::string("An instruction is being mutated twice! Aborting...");
            }
          }
		}
      }
    }
  }
  
  // notify change of program 
  return true;
}

Pass *
llvm::createMutatorPass() {
  return new Mutator();
}



