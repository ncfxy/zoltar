#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Compiler.h"
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
#include <iostream>

#include "indexManager.h"
#include "contextManager.h"
#include "registration.h"

using namespace llvm;
using std::ofstream;
using namespace std;

namespace {
  class InvStoreInstrumenter : public ModulePass {
    bool runOnModule(Module &M);
  public:
    static char ID;
    InvStoreInstrumenter() : ModulePass(ID) {}
  };
}

char InvStoreInstrumenter::ID = 0;
static RegisterPass<InvStoreInstrumenter>
RPinstrstores("invstore", "Instrument memory stores for invariant analysis");

bool InvStoreInstrumenter::runOnModule(Module &M) {

  cerr << "instrument: --- Store Invariant ---\n";

  Function *Main = M.getFunction("main");
  LLVMContext &C = M.getContext();
  
  if (Main == 0) {
    cerr << "WARNING: cannot insert store instrumentation into a module"
         << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  // Add library function prototypes
  Constant *StoreDoubleFn = M.getOrInsertFunction("_handleInvariantChangeDouble", 
                              Type::getVoidTy(C),   // returns void
                              Type::getInt32Ty(C),  // invTypeIndex
                              Type::getInt32Ty(C),  // invIndex
                              Type::getDoubleTy(C), // val
                              NULL);
  Constant *StoreIntegerFn = M.getOrInsertFunction("_handleInvariantChangeInt", 
                              Type::getVoidTy(C),   // returns void
                              Type::getInt32Ty(C),  // invTypeIndex
                              Type::getInt32Ty(C),  // invIndex
                              Type::getInt32Ty(C),  // val
                              NULL);
  Constant *StorePointerFn = M.getOrInsertFunction("_handleInvariantChangePtr", 
                              Type::getVoidTy(C),   // returns void
                              Type::getInt32Ty(C),  // invTypeIndex
                              Type::getInt32Ty(C),  // invIndex
                              PointerType::getUnqual(Type::getInt32Ty(C)),  // val
                              NULL);
  Constant *StoreUintFn = M.getOrInsertFunction("_handleInvariantChangeUInt", 
                              Type::getVoidTy(C),   // returns void
                              Type::getInt32Ty(C),  // invTypeIndex
                              Type::getInt32Ty(C),  // invIndex
                              Type::getInt32Ty(C),  // val
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
    	  /*TODO: solve DbgStopPointInst problem*/
    	  /*if(isa<DbgStopPointInst>(*I)) {
          DbgStopPointInst &DSPI = cast<DbgStopPointInst>(*I);
          
          llvm::GetConstantStringInfo(DSPI.getDirectory(), dir);
          llvm::GetConstantStringInfo(DSPI.getFileName(), file);
          line = DSPI.getLine();
        }*/

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
          if(InstType->isIntegerTy()) {
            Args[0] = ConstantInt::get(Type::getInt32Ty(C), invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::getInt32Ty(C), nInvariants++);
            Args[2] = CastInst::CreateIntegerCast(Val, Type::getInt32Ty(C), true, "st.cast", I);
            CallInst::Create(StoreIntegerFn, Args.begin(), Args.end(), "", I);
          } else if(InstType->isFloatingPointTy()) {
            Args[0] = ConstantInt::get(Type::getInt32Ty(C), invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::getInt32Ty(C), nInvariants++);
            Args[2] = CastInst::CreateFPCast(Val, Type::getDoubleTy(C), "st.cast", I);
            CallInst::Create(StoreDoubleFn, Args.begin(), Args.end(), "", I);
          } else if(isa<PointerType>(InstType)) {
            Args[0] = ConstantInt::get(Type::getInt32Ty(C), invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::getInt32Ty(C), nInvariants++);
            Args[2] = CastInst::CreatePointerCast(Val, PointerType::getUnqual(Type::getInt32Ty(C)), "st.cast", I);
            CallInst::Create(StorePointerFn, Args.begin(), Args.end(), "", I);
          } else {
            Args[0] = ConstantInt::get(Type::getInt32Ty(C), invariantTypeIndex);
            Args[1] = ConstantInt::get(Type::getInt32Ty(C), nInvariants++);
            Args[2] = ConstantInt::get(Type::getInt32Ty(C), 0);
            CallInst::Create(StoreUintFn, Args.begin(), Args.end(), "", I);
          }

        }
      }
    }
  }

  // add the registration of the instrumented invariants in the _registerAll() function
  addInvariantTypeRegistration(M, invariantTypeIndex, nInvariants, "Stores", 0);
  
  std::cerr << "instrument: " << nInvariants << " store operations instrumented\n";

  // notify change of program 
  return true;
}


