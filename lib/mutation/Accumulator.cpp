#include "llvm/Instructions.h"
#include "Operator.h"
#include "Util.h"

#include <iostream>
#include <cstdlib>

using namespace llvm;


// += --> = and the like
namespace {
	
	class NonConstantInc : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<NonConstantInc>
RONCI("no-accum", "Remove +=, -=, etc. accumulators and leave a plain = assignment");

char NonConstantInc::ID = 0;

bool
NonConstantInc::isCompatible(BasicBlock::iterator &I) {

    BinaryOperator *BOP;

    
	if (isAccumulator(I)) {
        BOP = reinterpret_cast<BinaryOperator*>(I->getOperand(0));
        
        Value *Inc = BOP->getOperand(1);
                    
        if ( ! isa<Constant>(Inc) ) {
            //std::cout << *I << "\n";
            return true;
        }
	}
	return false;
}

Value *
NonConstantInc::apply(BasicBlock::iterator &I){
    // Return the increment instead of the accum+increment
    BinaryOperator *BOP = reinterpret_cast<BinaryOperator*>(I->getOperand(0));
    Value *Inc = BOP->getOperand(1);
	return Inc;
}


