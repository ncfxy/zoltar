#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/CFG.h"

#include "Util.h"
#include "Operator.h"

#include <string>

using namespace llvm;


namespace {

  class MutationLister:public ModulePass {
    bool runOnModule(Module & M);
  public:
    static char ID;
      MutationLister():ModulePass((intptr_t) & ID) {
  }};

}

char MutationLister::ID = 0;

static RegisterPass <MutationLister>
RPmutarith("mutation-list", "List possible mutations");

bool MutationLister::runOnModule(Module & M) {

  unsigned siteId = 0;

  LocationInfo LOCI;

  OperatorManager * OMgr = OperatorManager::getInstance();
  OperatorInfoList oplst;

  //cerr << "{ ";
  // Loop through all functions within module
  for (Module::iterator F = M.begin(), ME = M.end(); F != ME; ++F) {

    if (F->isDeclaration()) {
      continue;
    }

    // Loop through all basic blocks within function
    for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {
      // Loop through all instructions within basic block

      BasicBlock * bb = B;
      if (B != F->begin() && (pred_begin(bb) == pred_end(bb)))
        continue;

      for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE; I++) {
        // remember information of the last known debug stoppoint
        LOCI.update(*I);

        // Do something
        OMgr->getCompatibleOperators(I, oplst);
        for (OperatorInfoList::iterator opi = oplst.begin(); opi != oplst.end(); opi++) {
          //cerr << siteId++ << ": (" << ffId << ", " << bbId << ", " << line << "), ";
          //cout << siteId - 1 << " " << (*opi)->operatorName << "\n";
          //cerr << dir << file << ":" << line << "\n";

          //MutationOperator *op = (*opi)->build();
          //Value *newv = op->apply(I);
          //cerr << *I << "\n" << *newv << "\n";
          
          cerr << siteId++ << " " << LOCI.getId() << ":" << (*opi)->operatorName << "\n";
        }
      }
      
    }
  }
  //cerr << "}";


  // notify program was not changed
  return false;
}
