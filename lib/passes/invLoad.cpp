#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Streams.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/ValueSymbolTable.h"
#include "llvm/Value.h"
#include "llvm/Support/CallSite.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/CFG.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Analysis/LoopInfo.h"
#include <time.h>

#include "indexManager.h"
#include "contextManager.h"
#include "registration.h"

using namespace llvm;

namespace {
  class InvLoadInstrumenter : public ModulePass {
    bool runOnModule(Module &M);
  public:
    static char ID;
    InvLoadInstrumenter() : ModulePass((intptr_t)&ID) {}
  };
}

char InvLoadInstrumenter::ID = 0;
static RegisterPass<InvLoadInstrumenter>
RPinstrloads("invload", "Instrument memory loads for invariant analysis");

bool InvLoadInstrumenter::runOnModule(Module &M) {
  Function *Main = M.getFunction("main");

  cerr << "instrument: --- Load Invariant ---\n";

  if (Main == 0) {
    cerr << "WARNING: cannot insert load instrumentation into a module"
         << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  // Add library function prototypes
  Constant *LoadDoubleFn = M.getOrInsertFunction("_handleInvariantChangeDouble", 
                              Type::VoidTy,   // returns void
                              Type::Int32Ty,  // invTypeIndex
                              Type::Int32Ty,  // invIndex
                              Type::DoubleTy, // val
                              NULL);
  Constant *LoadIntegerFn = M.getOrInsertFunction("_handleInvariantChangeInt", 
                              Type::VoidTy,   // returns void
                              Type::Int32Ty,  // invTypeIndex
                              Type::Int32Ty,  // invIndex
                              Type::Int32Ty,  // val
                              NULL);
  Constant *LoadPointerFn = M.getOrInsertFunction("_handleInvariantChangePtr", 
                              Type::VoidTy,   // returns void
                              Type::Int32Ty,  // invTypeIndex
                              Type::Int32Ty,  // invIndex
                              PointerType::getUnqual(Type::Int32Ty),  // val
                              NULL);
  Constant *LoadUintFn = M.getOrInsertFunction("_handleInvariantChangeUInt", 
                              Type::VoidTy,   // returns void
                              Type::Int32Ty,  // invTypeIndex
                              Type::Int32Ty,  // invIndex
                              Type::Int32Ty,  // val
                              NULL);

  TargetData targetData(&M);
  unsigned int nInvariants = 0;
  unsigned int invariantTypeIndex = IndexManager::getInvariantTypeIndex();
  
  // Loop through all functions within module
  for (Module::iterator F = M.begin(), ME = M.end(); F != ME; ++F) {
    std::string dir="-", file="-", name="-";
    int line = 0;
          
    // skip the _registerAll function
    if(F->getName()=="_registerAll")
      continue;

    // Loop through all basic blocks within function
    for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {
      // Loop through all instructions within basic block
      for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE; I++) {

        // remember information of the last known debug stoppoint
        if(isa<DbgStopPointInst>(*I)) {
          DbgStopPointInst &DSPI = cast<DbgStopPointInst>(*I);
          
          llvm::GetConstantStringInfo(DSPI.getDirectory(), dir);
          llvm::GetConstantStringInfo(DSPI.getFileName(), file);
          line = DSPI.getLine();
        }

        // Consider only load instructions
        if(isa<LoadInst>(*I)) {
          LoadInst &LD = cast<LoadInst>(*I);
          Value *Val = LD.getOperand(0);        // value to be loaded
          const Type *InstType = LD.getType();  // type of loaded value
          std::vector<Value*> Args(3);
          
          // try to get name of the loaded value:
          // "variableName"
          // "-" if it cannot be found
          if(Val->hasName()) {
            name = Val->getName();
          } else if(Val->getUnderlyingObject()->hasName()) {
            name = Val->getUnderlyingObject()->getName();
          } else {
            name = "-";
          }

          // add source context of this invariant to context file
          ContextManager::addInvariantTypeContext(
            invariantTypeIndex,     // invariantTypeIndex
            nInvariants,            // invariantIndex
            dir,                    // path
            file,                   // file
            line,                   // line
            name);                  // name
          
          // needs to be instrumented after the value has been loaded
          I++;

          // insert call to correct library function,
          // which depends on the type of the loaded value,
          // after the current load instruction
          if(InstType->isInteger()) {
            Args[0] = ConstantInt::get(Type::Int32Ty, invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::Int32Ty, nInvariants++);
            Args[2] = CastInst::createIntegerCast(&LD, Type::Int32Ty, true, "ld.cast", I);
            CallInst::Create(LoadIntegerFn, Args.begin(), Args.end(), "", I);
          } else if(InstType->isFloatingPoint()) {
            Args[0] = ConstantInt::get(Type::Int32Ty, invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::Int32Ty, nInvariants++);
            Args[2] = CastInst::createFPCast(&LD, Type::DoubleTy, "ld.cast", I);
            CallInst::Create(LoadDoubleFn, Args.begin(), Args.end(), "", I);
          } else if(isa<PointerType>(InstType)) {
            Args[0] = ConstantInt::get(Type::Int32Ty, invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::Int32Ty, nInvariants++);
            Args[2] = CastInst::createPointerCast(&LD, PointerType::getUnqual(Type::Int32Ty), "ld.cast", I);
            CallInst::Create(LoadPointerFn, Args.begin(), Args.end(), "", I);
          } else {
            Args[0] = ConstantInt::get(Type::Int32Ty, invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::Int32Ty, nInvariants++);
            Args[2] = ConstantInt::get(Type::Int32Ty, 0);
            CallInst::Create(LoadUintFn, Args.begin(), Args.end(), "", I);
          }

          I--;
        }
      }
    }
  }

  // add the registration of the instrumented invariants in the _registerAll() function
  addInvariantTypeRegistration(M, invariantTypeIndex, nInvariants, "Loads", 0);
  
  llvm::cerr << "instrument: " << nInvariants << " load operations instrumented\n";
  
  // notify change of program 
  return true;
}


