
#include "Util.h"
#include "llvm/Instructions.h"

bool
isAccumulator(Instruction *I) {
    StoreInst *SI;
    BinaryOperator *BOP;
    LoadInst *LI;
    
	if ( (SI = reinterpret_cast<StoreInst*>(I)) ) {
	    if ( (BOP = reinterpret_cast<BinaryOperator*>(SI->getOperand(0))) ) {
	        if ( (LI = reinterpret_cast<LoadInst*>(BOP->getOperand(0))) ) {
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
        BOP = reinterpret_cast<BinaryOperator*>(I->getOperand(0));
        
        if ( (C = reinterpret_cast<ConstantInt*>(BOP->getOperand(1))) ) {
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
	SI  = reinterpret_cast<StoreInst*>(I);
	BOP = reinterpret_cast<BinaryOperator*>(SI->getOperand(0));
	LI  = reinterpret_cast<LoadInst*>(BOP->getOperand(0));
	
	return LI;
}
