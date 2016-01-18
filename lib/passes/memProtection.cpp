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
#include <iostream>

using namespace llvm;
using namespace std;

namespace {
  class MemProtectionInstrumenter2 : public ModulePass {
    bool runOnModule(Module &M);
    Constant *getOrInsertGlobal(Module &M, const std::string &Name, const Type *Ty);
    void instrumentStore(LLVMContext &C, BasicBlock::iterator Inst, Value *loc, Value *len);
    Constant *memstart, *memend, *FailFn;
    TargetData *targetData;
  public:
    static char ID;
    MemProtectionInstrumenter2() : ModulePass(ID) {}
  };
}

char MemProtectionInstrumenter2::ID = 0;
static RegisterPass<MemProtectionInstrumenter2>
RPmemprotection("memprotection", "Instrument stores for memory protection");

bool MemProtectionInstrumenter2::runOnModule(Module &M) {

  cerr << "instrument: --- Memory Protection ---\n";
  
  Function *Main = M.getFunction("main");
  LLVMContext &C = M.getContext();

  if (Main == 0) {
    cerr << "WARNING: cannot insert block instrumentation into a module"
         << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  // Add library function prototypes
  FailFn = M.getOrInsertFunction("_handleMemFail", 
                              Type::getVoidTy(C),   // returns void
                              Type::getInt32Ty(C),  // loc
                              Type::getInt32Ty(C),  // size
                              NULL);
  memstart = getOrInsertGlobal(M, "_instrumentationInfo", PointerType::getUnqual(Type::getInt8Ty(C)));
  memend = getOrInsertGlobal(M, "_instrumentationInfoEnd", PointerType::getUnqual(Type::getInt8Ty(C)));

  // Instrument all store and memory intrinsic instructions
  unsigned int instrumentedStores = 0;
  targetData = new TargetData(&M);

  for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F) {
    if(F->isDeclaration()) // skip function declarations
      continue;

    int nStores = 0;
    bool done = false;
    while (!done) {
      int cnt = 0;
      for (Function::iterator BB = F->begin(), BBE = F->end(); BB != BBE; ++BB) {
        bool restart = false;
        for (BasicBlock::iterator I = BB->begin(); I != BB->end(); I++) {
          if (isa<StoreInst>(*I) || 
              isa<MemCpyInst>(*I) ||
              isa<MemMoveInst>(*I) ||
              isa<MemSetInst>(*I)) {
            if (cnt == nStores) {
              //found unprocessed store
              Value *Loc = NULL;
              Value *Len = NULL;
              if (isa<StoreInst>(*I)) {
                StoreInst &ST = cast<StoreInst>(*I);
                Loc = ST.getOperand(1);
                Value *Val = ST.getOperand(0);
                const Type *InstType = Val->getType();
                Len = ConstantInt::get(Type::getInt32Ty(C), targetData->getTypeStoreSize(InstType));
              } else if (isa<MemCpyInst>(*I)) {
                MemCpyInst &ST = cast<MemCpyInst>(*I);
                Loc = ST.getRawDest();
                Len = ST.getLength();
              } else if (isa<MemMoveInst>(*I)) {
                MemMoveInst &ST = cast<MemMoveInst>(*I);
                Loc = ST.getRawDest();
                Len = ST.getLength();
              } else if (isa<MemSetInst>(*I)) {
                MemSetInst &ST = cast<MemSetInst>(*I);
                Loc = ST.getRawDest();
                Len = ST.getLength();
              }

              instrumentStore(C, I, Loc, Len);

              nStores++;
              restart = true;
              break;
            } else {
              cnt++;
            }
          }
        }
        if (restart) {
          break;
        }
      }
      if (cnt == nStores) {
        done = true;
      }
    }
    instrumentedStores += nStores;
  }

  /*for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    for (Function::iterator BB = I->begin(), E = I->end(); BB != E; ++BB)
      for (BasicBlock::iterator Inst = BB->begin(); Inst != BB->end(); Inst++) {
        if(isa<StoreInst>(*Inst)) {
          StoreInst &ST = cast<StoreInst>(*Inst);
          //Value *Loc = CastInst::createPointerCast(ST.getOperand(1), Type::getInt32Ty(C), "st.cast", Inst);
          Value *LocTmp = ST.getOperand(1);
          Value *Loc = CastInst::createPointerCast(LocTmp, Type::getInt32Ty(C), "st.cast", Inst);
          Value *Val = ST.getOperand(0);
          const Type *InstType = Val->getType();
          int tsz = targetData.getTypeStoreSize(InstType);

          //STORE = splitBasicBlock(Inst)
          BasicBlock *bbStore = BB->splitBasicBlock(Inst, "bbStore");
          //CALL = splitBasicBlock(BB->end())
          BasicBlock *bbCall = BB->splitBasicBlock(--(BB->end()), "bbCall");
          //CMP2 = splitBasicBlock(BB->end())
          BasicBlock *bbCmp2 = BB->splitBasicBlock(--(BB->end()), "bbCmp2");

          //remainder of BB:
          //load memend
          Value *MemE = CastInst::createPointerCast(
            new LoadInst(memend, "loadMemEnd", &BB->back()),
            Type::getInt32Ty(C), "st.cast", &BB->back());
          //cmp loc>memend
          Value *VsGtMe = new ICmpInst(ICmpInst::ICMP_UGT, 
            Loc, 
            MemE,
            "VsGtMe", &BB->back());
          //if true branch to STORE
          BranchInst::Create(bbStore, bbCmp2, VsGtMe, &BB->back());
          BB->back().eraseFromParent();

          //BB CMP2:
          //load memstart
          Value *MemS = CastInst::createPointerCast(
            new LoadInst(memstart, "loadMemStart", &bbCmp2->back()),
            Type::getInt32Ty(C), "st.cast", &bbCmp2->back());
          //compute end=loc+tsz
          Value *ValE = BinaryOperator::Create(Instruction::Add, 
            Loc,
            ConstantInt::get(Type::getInt32Ty(C), tsz), "valE", &bbCmp2->back());
          //cmp end<memstart
          Value *VeLtMs = new ICmpInst(ICmpInst::ICMP_ULT, 
            ValE, 
            MemS,
            "VeLtMs", &bbCmp2->back());
          //if true branch to STORE
          BranchInst::Create(bbStore, bbCall, VeLtMs, &bbCmp2->back());
          bbCmp2->back().eraseFromParent();

          //BB Call:
          std::vector<Value*> Args(2);
          Args[0] = Loc;
          Args[1] = ConstantInt::get(Type::getInt32Ty(C), tsz);
          CallInst::Create(FailFn, Args.begin(), Args.end(), "", &bbCall->back());


          count++;

          cerr << "instrumented 1 store\n";
          return true;
        } else if (isa<MemCpyInst>(*Inst)) {
          MemCpyInst &ST = cast<MemCpyInst>(*Inst);
          Value *Loc = ST.getRawDest();
          Value *Len = ST.getLength();
          std::vector<Value*> Args(2);
          Args[0] = CastInst::createPointerCast(Loc, Type::getInt32Ty(C), "st.cast", Inst);
          Args[1] = CastInst::createIntegerCast(Len, Type::getInt32Ty(C), true, "st.cast", Inst);
          CallInst::Create(FailFn, Args.begin(), Args.end(), "", Inst);
          count++;
        } else if (isa<MemMoveInst>(*Inst)) {
          MemMoveInst &ST = cast<MemMoveInst>(*Inst);
          Value *Loc = ST.getRawDest();
          Value *Len = ST.getLength();
          std::vector<Value*> Args(2);
          Args[0] = CastInst::createPointerCast(Loc, Type::getInt32Ty(C), "st.cast", Inst);
          Args[1] = CastInst::createIntegerCast(Len, Type::getInt32Ty(C), true, "st.cast", Inst);
          CallInst::Create(FailFn, Args.begin(), Args.end(), "", Inst);
          count++;
        } else if (isa<MemSetInst>(*Inst)) {
          MemSetInst &ST = cast<MemSetInst>(*Inst);
          Value *Loc = ST.getRawDest();
          Value *Len = ST.getLength();
          std::vector<Value*> Args(2);
          Args[0] = CastInst::createPointerCast(Loc, Type::getInt32Ty(C), "st.cast", Inst);
          Args[1] = CastInst::createIntegerCast(Len, Type::getInt32Ty(C), true, "st.cast", Inst);
          CallInst::Create(FailFn, Args.begin(), Args.end(), "", Inst);
          count++;
        }
      }
*/
  // notify change of program 

  std::cerr << "instrument: " << instrumentedStores << " memory protection points instrumented\n";

  return true;
}

Constant *MemProtectionInstrumenter2::getOrInsertGlobal(Module &M, const std::string &Name, const Type *Ty) {
  GlobalVariable *GV = M.getGlobalVariable(Name);
  if(GV==0) {
    GlobalVariable *New = new GlobalVariable(M, Ty, false, GlobalVariable::ExternalLinkage, 0, Name);
		return New;
  }
  if(GV->getType() != PointerType::getUnqual(Ty)) {
    return ConstantExpr::getBitCast(GV, PointerType::getUnqual(Ty));
  }
  return GV;
}

void MemProtectionInstrumenter2::instrumentStore(LLVMContext &C, BasicBlock::iterator Inst, Value* loc, Value *len) {

  Value *Loc = CastInst::CreatePointerCast(loc, Type::getInt32Ty(C), "st.cast", Inst);
  Value *LenCast = len;
  if (Loc->getType() != len->getType()) {
    LenCast = CastInst::CreateIntegerCast(len, Type::getInt32Ty(C), false, "len.cast", Inst);
  }

  // create three basic blocks for branch targets
  BasicBlock *BB = Inst->getParent();
  BasicBlock *bbStore = BB->splitBasicBlock(Inst, "bbStore");
  BasicBlock *bbCall = BB->splitBasicBlock(--(BB->end()), "bbCall");
  BasicBlock *bbCmp2 = BB->splitBasicBlock(--(BB->end()), "bbCmp2");

  // remainder of BB:
  //  load memend
  Value *MemE = CastInst::CreatePointerCast(new LoadInst(memend, "loadMemEnd", &BB->back()),
                                            Type::getInt32Ty(C), 
                                            "st.cast", 
                                            &BB->back());
  //  cmp loc>memend
  Value *VsGtMe = new ICmpInst(&BB->back(),
                               ICmpInst::ICMP_UGT, 
                               Loc, 
                               MemE,
                               "VsGtMe");
  //  if true branch to STORE
  BranchInst::Create(bbStore, bbCmp2, VsGtMe, &BB->back());
  BB->back().eraseFromParent();

  // BB CMP2:
  //  load memstart
  Value *MemS = CastInst::CreatePointerCast(new LoadInst(memstart, "loadMemStart", &bbCmp2->back()),
                                            Type::getInt32Ty(C), 
                                            "st.cast", 
                                            &bbCmp2->back());
  //  compute end=loc+tsz

  Value *ValE = BinaryOperator::Create(Instruction::Add, 
                                       Loc,
                                       LenCast, "valE", &bbCmp2->back());
  //  cmp end<memstart
  Value *VeLtMs = new ICmpInst(&bbCmp2->back(),
                               ICmpInst::ICMP_ULT, 
                               ValE, 
                               MemS,
                              "VeLtMs");
  //  if true branch to STORE
  BranchInst::Create(bbStore, bbCall, VeLtMs, &bbCmp2->back());
  bbCmp2->back().eraseFromParent();

  // BB Call:
  std::vector<Value*> Args(2);
  Args[0] = Loc;
  Args[1] = LenCast;
  CallInst::Create(FailFn, Args.begin(), Args.end(), "", &bbCall->back());
  // continues to bbStore
}

