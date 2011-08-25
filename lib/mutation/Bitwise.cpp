#include "llvm/Instructions.h"
#include "Operator.h"

using namespace llvm;


// Anything to And operator
namespace {
	
	class And : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<And>
ROAnd("bitw-and", "Replace a bitwise operator by an and (a&b)");

char And::ID = 0;

bool
And::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::Or:
		case Instruction::Xor:
			return true;
		default:
			return false;
	}
}

Value *
And::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateAnd(I->getOperand(0), I->getOperand(1), "", I);
}

// Anything to Or operator
namespace {
	
	class Or : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<Or>
ROOr("bitw-or", "Replace a bitwise operator by an or (a|b)");

char Or::ID = 0;

bool
Or::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::And:
		case Instruction::Xor:
			return true;
		default:
			return false;
	}
}

Value *
Or::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateOr(I->getOperand(0), I->getOperand(1), "", I);
}

// Anything to Xor operator
namespace {
	
	class Xor : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<Xor>
ROXor("bitw-xor", "Replace bitwise operator by a xor (a^b)");

char Xor::ID = 0;

bool
Xor::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::And:
		case Instruction::Or:
			return true;
		default:
			return false;
	}
}

Value *
Xor::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateXor(I->getOperand(0), I->getOperand(1), "", I);
}

// Remove not
namespace {
	
	class NoNot : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<NoNot>
RONoNot("bitw-not", "Remove a bitwise not (~a -> a)");

char NoNot::ID = 0;

bool
NoNot::isCompatible(BasicBlock::iterator &I) {
	return BinaryOperator::isNot(I);
}

Value *
NoNot::apply(BasicBlock::iterator &I){
	return BinaryOperator::getNotArgument(I);
}



