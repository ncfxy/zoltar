#include "llvm/Instructions.h"
#include "Operator.h"

using namespace llvm;


// Eq operator
namespace {
	
  class Eq : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<Eq>
ROEq("comp-eq", "Replace relational operator by equality (a==b)");

char Eq::ID = 0;

bool
Eq::isCompatible(BasicBlock::iterator &I) {
	if (CmpInst *CI = dynamic_cast<CmpInst*>(&(*I))) {
		return CI->getPredicate() != CmpInst::ICMP_EQ &&
		       CI->getPredicate() != CmpInst::FCMP_OEQ &&
		       CI->getPredicate() != CmpInst::FCMP_UEQ;
	}
	return false;
}

Value *
Eq::apply(BasicBlock::iterator &I){
    if (ICmpInst *ICI = dynamic_cast<ICmpInst*>(&(*I))) {
        return new ICmpInst(I,
                            CmpInst::ICMP_EQ, 
                            ICI->getOperand(0), ICI->getOperand(1));
    }
    if (FCmpInst *FCI = dynamic_cast<FCmpInst*>(&(*I))) {
        return new FCmpInst(I,
                            FCI->getPredicate() < CmpInst::FCMP_UNO ? CmpInst::FCMP_OEQ : CmpInst::FCMP_UEQ, 
                            FCI->getOperand(0), FCI->getOperand(1));
    }
	return 0;
}

// NEq operator
namespace {
	
  class NEq : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<NEq>
RONEq("comp-neq", "Replace relational operator by inequality (a!=b)");

char NEq::ID = 0;

bool
NEq::isCompatible(BasicBlock::iterator &I) {
	if (CmpInst *CI = dynamic_cast<CmpInst*>(&(*I))) {
		return CI->getPredicate() != CmpInst::ICMP_NE &&
		       CI->getPredicate() != CmpInst::FCMP_ONE &&
		       CI->getPredicate() != CmpInst::FCMP_UNE;
	}
	return false;
}

Value *
NEq::apply(BasicBlock::iterator &I){
    if (ICmpInst *ICI = dynamic_cast<ICmpInst*>(&(*I))) {
        return new ICmpInst(I,
                            CmpInst::ICMP_NE, 
                            ICI->getOperand(0), ICI->getOperand(1));
    }
    if (FCmpInst *FCI = dynamic_cast<FCmpInst*>(&(*I))) {
        return new FCmpInst(I,
                            FCI->getPredicate() < CmpInst::FCMP_UNO ? CmpInst::FCMP_ONE : CmpInst::FCMP_UNE, 
                            FCI->getOperand(0), FCI->getOperand(1));
    }
	return 0;
}

// Gt operator
namespace {
	
  class Gt : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<Gt>
ROGt("comp-gt", "Replace relational operator by greater than (a<b)");

char Gt::ID = 0;

bool
Gt::isCompatible(BasicBlock::iterator &I) {
	if (CmpInst *CI = dynamic_cast<CmpInst*>(&(*I))) {
		return CI->getPredicate() != CmpInst::ICMP_UGT &&
               CI->getPredicate() != CmpInst::ICMP_SGT &&
		       CI->getPredicate() != CmpInst::FCMP_OGT &&
		       CI->getPredicate() != CmpInst::FCMP_UGT;
	}
	return false;
}

Value *
Gt::apply(BasicBlock::iterator &I){
    if (ICmpInst *ICI = dynamic_cast<ICmpInst*>(&(*I))) {
        return new ICmpInst(I,
                            ICI->getPredicate() < CmpInst::ICMP_SGT ? CmpInst::ICMP_UGT : CmpInst::ICMP_SGT, 
                            ICI->getOperand(0), ICI->getOperand(1));
    }
    if (FCmpInst *FCI = dynamic_cast<FCmpInst*>(&(*I))) {
        return new FCmpInst(I,
                            FCI->getPredicate() < CmpInst::FCMP_UNO ? CmpInst::FCMP_OGT : CmpInst::FCMP_UGT, 
                            FCI->getOperand(0), FCI->getOperand(1));
    }
	return 0;
}

// GEq operator
namespace {
	
  class GEq : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<GEq>
ROGEq("comp-geq", "Replace relational operator by greater or equal than (a>=b)");

char GEq::ID = 0;

bool
GEq::isCompatible(BasicBlock::iterator &I) {
	if (CmpInst *CI = dynamic_cast<CmpInst*>(&(*I))) {
		return CI->getPredicate() != CmpInst::ICMP_UGE &&
               CI->getPredicate() != CmpInst::ICMP_SGE &&
		       CI->getPredicate() != CmpInst::FCMP_OGE &&
		       CI->getPredicate() != CmpInst::FCMP_UGE;
	}
	return false;
}

Value *
GEq::apply(BasicBlock::iterator &I){
    if (ICmpInst *ICI = dynamic_cast<ICmpInst*>(&(*I))) {
        return new ICmpInst(I,
                            ICI->getPredicate() < CmpInst::ICMP_SGT ? CmpInst::ICMP_UGE : CmpInst::ICMP_SGE, 
                            ICI->getOperand(0), ICI->getOperand(1));
    }
    if (FCmpInst *FCI = dynamic_cast<FCmpInst*>(&(*I))) {
        return new FCmpInst(I,
                            FCI->getPredicate() < CmpInst::FCMP_UNO ? CmpInst::FCMP_OGE : CmpInst::FCMP_UGE, 
                            FCI->getOperand(0), FCI->getOperand(1));
    }
	return 0;
}

// Lt operator
namespace {
	
  class Lt : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<Lt>
ROLt("comp-lt", "Replace relational operator by lower than (a<b)");

char Lt::ID = 0;

bool
Lt::isCompatible(BasicBlock::iterator &I) {
	if (CmpInst *CI = dynamic_cast<CmpInst*>(&(*I))) {
		return CI->getPredicate() != CmpInst::ICMP_ULT &&
               CI->getPredicate() != CmpInst::ICMP_SLT &&
		       CI->getPredicate() != CmpInst::FCMP_OLT &&
		       CI->getPredicate() != CmpInst::FCMP_ULT;
	}
	return false;
}

Value *
Lt::apply(BasicBlock::iterator &I){
    if (ICmpInst *ICI = dynamic_cast<ICmpInst*>(&(*I))) {
        return new ICmpInst(I,
                            ICI->getPredicate() < CmpInst::ICMP_SGT ? CmpInst::ICMP_ULT : CmpInst::ICMP_SLT, 
                            ICI->getOperand(0), ICI->getOperand(1));
    }
    if (FCmpInst *FCI = dynamic_cast<FCmpInst*>(&(*I))) {
        return new FCmpInst(I,
                            FCI->getPredicate() < CmpInst::FCMP_UNO ? CmpInst::FCMP_OLT : CmpInst::FCMP_ULT, 
                            FCI->getOperand(0), FCI->getOperand(1));
    }
	return 0;
}

// LEq operator
namespace {
	
  class LEq : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<LEq>
ROLEq("comp-le", "Replace relational operator by lower or equal than (a<=b)");

char LEq::ID = 0;

bool
LEq::isCompatible(BasicBlock::iterator &I) {
	if (CmpInst *CI = dynamic_cast<CmpInst*>(&(*I))) {
		return CI->getPredicate() != CmpInst::ICMP_ULE &&
               CI->getPredicate() != CmpInst::ICMP_SLE &&
		       CI->getPredicate() != CmpInst::FCMP_OLE &&
		       CI->getPredicate() != CmpInst::FCMP_ULE;
	}
	return false;
}

Value *
LEq::apply(BasicBlock::iterator &I){
    if (ICmpInst *ICI = dynamic_cast<ICmpInst*>(&(*I))) {
        return new ICmpInst(I,
                            ICI->getPredicate() < CmpInst::ICMP_SGT ? CmpInst::ICMP_ULE : CmpInst::ICMP_SLE, 
                            ICI->getOperand(0), ICI->getOperand(1));
    }
    if (FCmpInst *FCI = dynamic_cast<FCmpInst*>(&(*I))) {
        return new FCmpInst(I,
                            FCI->getPredicate() < CmpInst::FCMP_UNO ? CmpInst::FCMP_OLE : CmpInst::FCMP_ULE, 
                            FCI->getOperand(0), FCI->getOperand(1));
    }
	return 0;
}

