#include "llvm/Instructions.h"
#include "Operator.h"

#include <iostream>

using namespace llvm;


// Anything to And operator
namespace {
	
	class Idx : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<Idx>
ROIdx("arr-idx", "Offset array indexing operations by 1");

char Idx::ID = 0;

bool
Idx::isCompatible(BasicBlock::iterator &I) {
    if (GetElementPtrInst *GEPI = reinterpret_cast<GetElementPtrInst*>(&(*I))) {
        return GEPI->getNumIndices() == 1;
    }
	return false;
}

Value *
Idx::apply(BasicBlock::iterator &I){
    if (GetElementPtrInst *GEPI = reinterpret_cast<GetElementPtrInst*>(&(*I))) {
        Value *idx = GEPI->getOperand(1);
        Value * ct = ConstantInt::get(idx->getType(), 1);
        Value *bop = BinaryOperator::CreateAdd(idx, ct, "mut.inc", GEPI);
        
        GetElementPtrInst *newi = GetElementPtrInst::Create(GEPI->getOperand(0), bop, "mut.gep", GEPI);
        
        return newi;
    }
	return 0;
}

