
#include "Util.h"
#include "llvm/Instructions.h"

bool
isAccumulator(Instruction *I) {
    StoreInst *SI;
    BinaryOperator *BOP;
    LoadInst *LI;
    
	if ( (SI = dynamic_cast<StoreInst*>(I)) ) {
	    if ( (BOP = dynamic_cast<BinaryOperator*>(SI->getOperand(0))) ) {
	        if ( (LI = dynamic_cast<LoadInst*>(BOP->getOperand(0))) ) {
                Value *st_addr = SI->getPointerOperand();
                Value *ld_addr = LI->getPointerOperand();

                if (ld_addr == st_addr) {
                    return true;
                }
	        }
	    }
    }
    return false;
}

bool
isIncrementOrDecrement(Instruction *I) {
    
    BinaryOperator *BOP;
    ConstantInt *C;
    
    if (isAccumulator(I) ) {
        BOP = dynamic_cast<BinaryOperator*>(I->getOperand(0));
        
        if ( (C = dynamic_cast<ConstantInt*>(BOP->getOperand(1))) ) {
            return C->isOne();
	    }
    }
    return false;
}

Value *
getOldValue(Instruction *I) {
    StoreInst *SI;
    BinaryOperator *BOP;
    LoadInst *LI;
	SI  = dynamic_cast<StoreInst*>(I);
	BOP = dynamic_cast<BinaryOperator*>(SI->getOperand(0));
	LI  = dynamic_cast<LoadInst*>(BOP->getOperand(0));
	
	return LI;
}
