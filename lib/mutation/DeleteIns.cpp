#include "llvm/Instructions.h"
#include "Operator.h"
#include "Util.h"

#include <iostream>

using namespace llvm;


// += --> = and the like
namespace {
	
  class DeleteAssignment : public MutationOperator {
    public:
      static char ID;
      static bool isCompatible(BasicBlock::iterator &I);
    public:
      Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<DeleteAssignment>
RODel("no-ass", "Remove an assignment (skip a variable store)");

char DeleteAssignment::ID = 0;

bool
DeleteAssignment::isCompatible(BasicBlock::iterator &I) {
    return isa<StoreInst>(*I);
}

Value *
DeleteAssignment::apply(BasicBlock::iterator &I){
    // Remove and return NULL
    BasicBlock *BB = I->getParent();
    I = BB->getInstList().erase(I);
	return NULL;
}


