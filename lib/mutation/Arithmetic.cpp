#include "llvm/Instructions.h"
#include "Operator.h"

using namespace llvm;


// Anything to Add operator
namespace {
	
	class Add : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<Add>
ROAdd("math-add", "Replace math operator by an addition (+)");

char Add::ID = 0;

bool
Add::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::Sub:
		case Instruction::Mul:
		case Instruction::UDiv:
		case Instruction::SDiv:
		case Instruction::URem:
		case Instruction::SRem:
			return true;
		default:
			return false;
	}
}

Value *
Add::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateAdd(I->getOperand(0), I->getOperand(1), "", I);
}

// Anything to Sub operator
namespace {
	
	class Sub : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<Sub>
ROSub("math-sub", "Replace math operator by a substraction (-)");

char Sub::ID = 0;

bool
Sub::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::Add:
		case Instruction::Mul:
		case Instruction::UDiv:
		case Instruction::SDiv:
		case Instruction::URem:
		case Instruction::SRem:
			return true;
		default:
			return false;
	}
}

Value *
Sub::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateSub(I->getOperand(0), I->getOperand(1), "", I);
}

// Anything to Mul operator
namespace {
	
	class Mul : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<Mul>
ROMul("math-mul", "Replace math operator by a multiplication (*)");

char Mul::ID = 0;

bool
Mul::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::Add:
		case Instruction::Sub:
		case Instruction::UDiv:
		case Instruction::SDiv:
		case Instruction::URem:
		case Instruction::SRem:
			return true;
		default:
			return false;
	}
}

Value *
Mul::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateMul(I->getOperand(0), I->getOperand(1), "", I);
}

// Anything to UDiv
namespace {
	
	class UDiv : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<UDiv>
ROUDiv("math-udiv", "Replace math operator by a unsigned division (/)");

char UDiv::ID = 0;

bool
UDiv::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::Add:
		case Instruction::Mul:
		case Instruction::Sub:
		case Instruction::URem:
		case Instruction::SRem:
			return true;
		default:
			return false;
	}
}

Value *
UDiv::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateUDiv(I->getOperand(0), I->getOperand(1), "", I);
}

// Anything to SDiv
namespace {
	
	class SDiv : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<SDiv>
ROSDiv("math-sdiv", "Replace math operator by a signed division (/)");

char SDiv::ID = 0;

bool
SDiv::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::Add:
		case Instruction::Mul:
		case Instruction::Sub:
		case Instruction::URem:
		case Instruction::SRem:
			return true;
		default:
			return false;
	}
}

Value *
SDiv::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateSDiv(I->getOperand(0), I->getOperand(1), "", I);
}

// Anything to URem
namespace {
	
	class URem : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<URem>
ROURem("math-urem", "Replace math operator by an unsigned remainder (%)");

char URem::ID = 0;

bool
URem::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::Add:
		case Instruction::Mul:
		case Instruction::Sub:
		case Instruction::UDiv:
		case Instruction::SDiv:
			return true;
		default:
			return false;
	}
}

Value *
URem::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateURem(I->getOperand(0), I->getOperand(1), "", I);
}

// Anything to SRem
namespace {
	
	class SRem : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<SRem>
ROSRem("math-srem", "Replace math operator by a signed remainder (%)");

char SRem::ID = 0;

bool
SRem::isCompatible(BasicBlock::iterator &I) {
	switch (I->getOpcode()) {
		case Instruction::Add:
		case Instruction::Mul:
		case Instruction::Sub:
		case Instruction::UDiv:
		case Instruction::SDiv:
			return true;
		default:
			return false;
	}
}

Value *
SRem::apply(BasicBlock::iterator &I){
	return BinaryOperator::CreateSRem(I->getOperand(0), I->getOperand(1), "", I);
}

// Remove Negation
namespace {
	
	class NoNeg : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<NoNeg>
RONoNeg("math-neg", "Remove a sign negation (-a)");

char NoNeg::ID = 0;

bool
NoNeg::isCompatible(BasicBlock::iterator &I) {
    return BinaryOperator::isNeg(I);
}

Value *
NoNeg::apply(BasicBlock::iterator &I){
	return BinaryOperator::getNegArgument(I);
}

// Remove FP Negation
namespace {
	
	class NoFNeg : public MutationOperator {
  public:
    static char ID;
		static bool isCompatible(BasicBlock::iterator &I);
  public:
    Value* apply(BasicBlock::iterator &I);
  };
	
}

static RegisterOperator<NoFNeg>
RONoFNeg("math-fneg", "Remove a floating point sign negation (-a)");

char NoFNeg::ID = 0;

bool
NoFNeg::isCompatible(BasicBlock::iterator &I) {
    return BinaryOperator::isFNeg(I);
}

Value *
NoFNeg::apply(BasicBlock::iterator &I){
    return BinaryOperator::getFNegArgument(I);
}


