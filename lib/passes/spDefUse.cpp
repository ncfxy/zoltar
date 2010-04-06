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
#include "llvm/CodeGen/MachineModuleInfo.h"
#include <time.h>
#include <fstream>
#include <sstream>

#include "indexManager.h"
#include "contextManager.h"
#include "registration.h"

using namespace llvm;
using std::ofstream;

namespace {
  class SpDefUseInstrumenter : public ModulePass {
    bool runOnModule(Module &M);
  public:
    static char ID;
    SpDefUseInstrumenter() : ModulePass((intptr_t)&ID) {}
  };
}


char SpDefUseInstrumenter::ID = 0;
static RegisterPass<SpDefUseInstrumenter>
RPspblock("spdefuse", "Insert instrumentation for def-use pair spectrum");

bool SpDefUseInstrumenter::runOnModule(Module &M) {

  cerr << "instrument: --- Def-Use pair Spectrum ---\n";

  Function *Main = M.getFunction("main");
  if (Main == 0) {
    cerr << "WARNING: cannot insert def-use instrumentation into a module"
         << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  // Add library function prototype
  Constant *SpFn = M.getOrInsertFunction("_updateSpectrum", 
                          Type::VoidTy, 
                          Type::Int32Ty,  // spectrum index
                          Type::Int32Ty,  // component index
                          NULL);


  unsigned spectrumIndex = IndexManager::getSpectrumIndex();
  unsigned nDefs = 0;
  unsigned nUses = 0;

  // Loop through all functions within module
  for (Module::iterator F = M.begin(), ME = M.end(); F != ME; ++F) {

    // skip function declarations
    if(F->isDeclaration()) 
      continue;

    // skip the _registerAll function
    if(F->getName()=="_registerAll")
      continue;
    
    // Loop through all basic blocks within function
    for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {
      //skip dead blocks
      //is this really safe??
      BasicBlock *bb = B;
      if (B!=F->begin() && (pred_begin(bb)==pred_end(bb))) continue; //skip dead blocks

      // Loop through all instructions within basic block
      for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE; I++) {
    
        if(isa<DbgDeclareInst>(*I)) {

          // extract source file information from debug intrinsic
          DbgDeclareInst &DDI = cast<DbgDeclareInst>(*I);
          std::string file, dir;
          std::string name;
          GlobalVariable *gv = cast<GlobalVariable>(DDI.getVariable());
          if(!gv->hasInitializer()) continue;
          ConstantStruct *cs = cast<ConstantStruct>(gv->getInitializer());
          llvm::GetConstantStringInfo(cs->getOperand(2), name);
          unsigned int line = unsigned(cast<ConstantInt>(cs->getOperand(4))->getZExtValue());
          Value *V = cast<Value>(cs->getOperand(3));
          GlobalVariable *gv2 = cast<GlobalVariable>(cast<ConstantExpr>(V)->getOperand(0));
          if(!gv2->hasInitializer()) continue;
          ConstantStruct *cs2 = cast<ConstantStruct>(gv2->getInitializer());
          llvm::GetConstantStringInfo(cs2->getOperand(3), file);
          llvm::GetConstantStringInfo(cs2->getOperand(4), dir);

          // get the allocation instruction of the variable definition
          AllocaInst *AI;
          if(isa<AllocaInst>(DDI.getAddress())) {
            AI = cast<AllocaInst>(DDI.getAddress());
          } else if (isa<BitCastInst>(DDI.getAddress())) {
            AI = cast<AllocaInst>(cast<BitCastInst>(DDI.getAddress())->getOperand(0));
          } else {
            continue;
          }

          nDefs++;

          // add calls to lib function for each use of the variable
          int currUses = 0;
          for(AllocaInst::use_iterator U = AI->use_begin(), UE = AI->use_end(); U != UE; ++U) {
            if(isa<Instruction>(*U)) {

              User *user = *U;
              Instruction *insertInst = (Instruction*)user;

              // find most likely context location of use
              int useline = line;
              std::string usefile = file, usedir = dir;
              BasicBlock *parent = insertInst->getParent();
              BasicBlock::iterator inst = parent->begin();
              while(((Instruction *)inst) != insertInst  &&  inst != parent->end()) {
                if(isa<DbgStopPointInst>(*inst)) {
                  DbgStopPointInst &DSPI = cast<DbgStopPointInst>(*inst);
                  llvm::GetConstantStringInfo(DSPI.getDirectory(), usedir);
                  llvm::GetConstantStringInfo(DSPI.getFileName(), usefile);
                  useline = DSPI.getLine();
                }
                inst++;
              }

              std::stringstream usename;
              usename << name << "(use_" << currUses << ")";
              // add source context of this invariant to context file
              ContextManager::addSpectrumContext(
                spectrumIndex,  // spectrumIndex
                nUses,          // componentIndex
                usedir,         // path
                usefile,        // file
                useline,        // line
                usename.str()); // name
              currUses++;

              std::vector<Value*> Args(2);
              Args[0] = ConstantInt::get(Type::Int32Ty, spectrumIndex);
              Args[1] = ConstantInt::get(Type::Int32Ty, nUses++);

              CallInst::Create(SpFn, Args.begin(), Args.end(), "", insertInst);
            }
          }
        }
      }
    }
  }

  // add the registration of the instrumented spectrum points in the _registerAll() function
  addSpectrumRegistration(M, spectrumIndex, nUses, "Def-Use_Pairs");
  
  llvm::cerr << "instrument: " << nDefs << " defines with a total number of " << nUses << " uses instrumented\n";

  // notify change of program 
  return true;
}


