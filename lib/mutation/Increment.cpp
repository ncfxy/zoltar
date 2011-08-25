#include "llvm/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "Operator.h"
#include "Util.h"

#include <iostream>

using namespace llvm;


// a++ --> ++a and the like
namespace {
	
	class PreIncrement : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<PreIncrement>
ROPreInc("pre-inc", "Switch from pre-increment (++a) to post-increment (a++) (also decrement)");

char PreIncrement::ID = 0;

bool
PreIncrement::isCompatible(BasicBlock::iterator &I) {
    StoreInst *SI;
    LoadInst *NLI;
    
    BasicBlock *BB = I->getParent();
    
    if (BB->begin() == I) 
        return false;
    
    BasicBlock::iterator Prev = I;
    Prev--;
    
	if ( isIncrementOrDecrement(Prev) ) {
	    //std::cout << *I << "\n";
        
        SI  = dynamic_cast<StoreInst*>(&(*Prev));
        Value *st_addr = SI->getPointerOperand();
        
        NLI = dynamic_cast<LoadInst*>(&(*I));
        if (NLI != NULL) {
            Value *ld_addr = NLI->getPointerOperand();
                        
    	    return st_addr == ld_addr;
    	}
	}
	return false;
}

Value *
PreIncrement::apply(BasicBlock::iterator &I){
    BasicBlock::iterator Prev = I;
    Prev--;
	return getOldValue(Prev);
}

namespace {
	
	class PostIncrement : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<PostIncrement>
ROPostInc("post-inc", "Switch from post-increment (a++) to pre-increment (++a) (also decrement)");

char PostIncrement::ID = 0;

bool
PostIncrement::isCompatible(BasicBlock::iterator &I) {
    StoreInst *SI;
    LoadInst *NLI;
    
    BasicBlock *BB = I->getParent();
    
    if (BB->begin() == I) 
        return false;
       
	if ( isIncrementOrDecrement(I) ) {
	    BasicBlock::iterator N = I;
	    N++;
	    
	    SI = dynamic_cast<StoreInst*>(&(*I));
	    //std::cout << *I << "\n";
	    Value *st_addr = SI->getPointerOperand();
	    
        if ( NLI = dynamic_cast<LoadInst*>(&(*N)) ) { //check it's not preinc
            Value *ld_addr = NLI->getPointerOperand();
                            
	        if (st_addr == ld_addr)
	            return false;
	    }
	    N--;N--;N--; //go to the load of the increment
	    Value *LI = FindAvailableLoadedValue(st_addr, BB, N, 0, NULL);
	    
	    if (LI != NULL) {
	        return true;
	    }
	    
	}  
	return false;
}

Value *
PostIncrement::apply(BasicBlock::iterator &I){
    BasicBlock *BB = I->getParent();
    
    StoreInst *SI = dynamic_cast<StoreInst*>(&(*I));
    BasicBlock::iterator F = I;F--;F--;
    Value *st_addr = SI->getPointerOperand();
    Value *LI = FindAvailableLoadedValue(st_addr, BB, F, 0, NULL);
    BasicBlock::iterator where = BB->begin();
    while (LI != where) where++;
    
    BasicBlock::InstListType &List = BB->getInstList();
    
    BasicBlock::InstListType Temp;
    
    Temp.push_front(List.remove(I--));
    Temp.push_front(List.remove(I--));
    Temp.push_front(List.remove(I--));

      
    BB->getInstList().splice(where, Temp);
    
    return NULL;
}



