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
#include <fstream>

#include "indexManager.h"
#include "contextManager.h"
#include "registration.h"

using namespace llvm;
using std::ofstream;

namespace {
  class InvStoreInstrumenter : public ModulePass {
    bool runOnModule(Module &M);
  public:
    static char ID;
    InvStoreInstrumenter() : ModulePass((intptr_t)&ID) {}
  };
}

char InvStoreInstrumenter::ID = 0;
static RegisterPass<InvStoreInstrumenter>
RPinstrstores("invstore", "Instrument memory stores for invariant analysis");

bool InvStoreInstrumenter::runOnModule(Module &M) {

  cerr << "instrument: --- Store Invariant ---\n";

  Function *Main = M.getFunction("main");
  if (Main == 0) {
    cerr << "WARNING: cannot insert store instrumentation into a module"
         << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  // Add library function prototypes
  Constant *StoreDoubleFn = M.getOrInsertFunction("_handleInvariantChangeDouble", 
                              Type::VoidTy,   // returns void
                              Type::Int32Ty,  // invTypeIndex
                              Type::Int32Ty,  // invIndex
                              Type::DoubleTy, // val
                              NULL);
  Constant *StoreIntegerFn = M.getOrInsertFunction("_handleInvariantChangeInt", 
                              Type::VoidTy,   // returns void
                              Type::Int32Ty,  // invTypeIndex
                              Type::Int32Ty,  // invIndex
                              Type::Int32Ty,  // val
                              NULL);
  Constant *StorePointerFn = M.getOrInsertFunction("_handleInvariantChangePtr", 
                              Type::VoidTy,   // returns void
                              Type::Int32Ty,  // invTypeIndex
                              Type::Int32Ty,  // invIndex
                              PointerType::getUnqual(Type::Int32Ty),  // val
                              NULL);
  Constant *StoreUintFn = M.getOrInsertFunction("_handleInvariantChangeUInt", 
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

        // Consider only store instructions
        if(isa<StoreInst>(*I)) {
          StoreInst &ST = cast<StoreInst>(*I);
          Value *Val = ST.getOperand(0);         // stored value
          Value *Target = ST.getOperand(1);      // target address
          const Type *InstType = Val->getType(); // type of value
          std::vector<Value*> Args(3);

          // try to get name of the stored value:
          // "variableName"
          // "-" if it cannot be found
          if(Target->hasName()) {
            name = Target->getName();
          } else if(Target->getUnderlyingObject()->hasName()) {
            name = Target->getUnderlyingObject()->getName();
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
          
          // insert call to correct library function,
          // which depends on the type of the stored value,
          // before the current store instruction
          if(InstType->isInteger()) {
            Args[0] = ConstantInt::get(Type::Int32Ty, invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::Int32Ty, nInvariants++);
            Args[2] = CastInst::createIntegerCast(Val, Type::Int32Ty, true, "st.cast", I);
            CallInst::Create(StoreIntegerFn, Args.begin(), Args.end(), "", I);
          } else if(InstType->isFloatingPoint()) {
            Args[0] = ConstantInt::get(Type::Int32Ty, invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::Int32Ty, nInvariants++);
            Args[2] = CastInst::createFPCast(Val, Type::DoubleTy, "st.cast", I);
            CallInst::Create(StoreDoubleFn, Args.begin(), Args.end(), "", I);
          } else if(isa<PointerType>(InstType)) {
            Args[0] = ConstantInt::get(Type::Int32Ty, invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::Int32Ty, nInvariants++);
            Args[2] = CastInst::createPointerCast(Val, PointerType::getUnqual(Type::Int32Ty), "st.cast", I);
            CallInst::Create(StorePointerFn, Args.begin(), Args.end(), "", I);
          } else {
            Args[0] = ConstantInt::get(Type::Int32Ty, invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::Int32Ty, nInvariants++);
            Args[2] = ConstantInt::get(Type::Int32Ty, 0);
            CallInst::Create(StoreUintFn, Args.begin(), Args.end(), "", I);
          }

        }
      }
    }
  }

  // add the registration of the instrumented invariants in the _registerAll() function
  addInvariantTypeRegistration(M, invariantTypeIndex, nInvariants, "Stores", 0);
  
  llvm::cerr << "instrument: " << nInvariants << " store operations instrumented\n";

  // notify change of program 
  return true;
}


