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
  class SpStatementInstrumenter : public ModulePass {
    bool runOnModule(Module &M);
  public:
    static char ID;
    SpStatementInstrumenter() : ModulePass(ID) {}
  };
}


char SpStatementInstrumenter::ID = 0;
static RegisterPass<SpStatementInstrumenter>
RPspblock("spstatement", "Insert instrumentation for statement hit spectrum");

bool SpStatementInstrumenter::runOnModule(Module &M) {

  cerr << "instrument: --- Statement Spectrum ---\n";

  Function *Main = M.getFunction("main");
  LLVMContext &C = M.getContext();
  
  if (Main == 0) {
    cerr << "WARNING: cannot insert statement instrumentation into a module"
         << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  // Add library function prototype
  Constant *SpFn = M.getOrInsertFunction("_updateSpectrum", 
                          Type::getVoidTy(C), 
                          Type::getInt32Ty(C),  // spectrum index
                          Type::getInt32Ty(C),  // component index
                          NULL);


  unsigned spectrumIndex = IndexManager::getSpectrumIndex();
  unsigned maxComponents = 0;
  
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

    	  /*TODO: solve DbgStopPointInst problem*/
        /*if(isa<DbgStopPointInst>(*I)) {
          DbgStopPointInst &DSPI = cast<DbgStopPointInst>(*I);
          std::string file, dir, name="-";
          llvm::GetConstantStringInfo(DSPI.getDirectory(), dir);
          llvm::GetConstantStringInfo(DSPI.getFileName(), file);
          unsigned line = DSPI.getLine();

          // add source context of this invariant to context file
          ContextManager::addSpectrumContext(
            spectrumIndex,     // spectrumIndex
            line,              // componentIndex
            dir,               // path
            file,              // file
            line,              // line
            name);             // name
  
          // add call to lib function
          std::vector<Value*> Args(2);
          Args[0] = ConstantInt::get(Type::getInt32Ty(C), spectrumIndex);
          Args[1] = ConstantInt::get(Type::getInt32Ty(C), line);
          
          CallInst::Create(SpFn, Args.begin(), Args.end(), "", I);
          
          if (maxComponents < line) {
            maxComponents = line;
          }
          maxComponents += 1; // base 0
        }*/
      }
    }
  }

  // add the registration of the instrumented spectrum points in the _registerAll() function
  addSpectrumRegistration(M, spectrumIndex, maxComponents, "Statement");
  
  std::cerr << "instrument: " << maxComponents << " statements instrumented\n";

  // notify change of program 
  return true;
}


