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
  class SpFunctionInstrumenter : public ModulePass {
    int numFunctionsInstrumented;
    bool runOnModule(Module &M);
  public:
    static char ID;
    SpFunctionInstrumenter() : ModulePass((intptr_t)&ID) {numFunctionsInstrumented = 0;}
  };
}


char SpFunctionInstrumenter::ID = 0;
static RegisterPass<SpFunctionInstrumenter>
RPspfunction("spfunction", "Insert instrumentation for function spectrum");

bool SpFunctionInstrumenter::runOnModule(Module &M) {

  cerr << "instrument: --- Function Spectrum ---\n";

  Function *Main = M.getFunction("main");
  if (Main == 0) {
    cerr << "WARNING: cannot insert function instrumentation into a module"
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
  unsigned nComponents = 0;

  // Loop through all functions within module
  for (Module::iterator F = M.begin(), ME = M.end(); F != ME; ++F) {

    // skip function declarations
    if(F->isDeclaration()) 
      continue;

    // skip the _registerAll function
    if(F->getName()=="_registerAll")
      continue;
    
    Function::iterator fit = F->begin();
    BasicBlock::iterator bbit = fit->begin();

    while(!isa<DbgFuncStartInst>(*bbit)) ++bbit;

    // extract source file information from debug instrinsic
    DbgFuncStartInst &dfsi = cast<DbgFuncStartInst>(*bbit);
    Value *subProgram = dfsi.getSubprogram();
    GlobalVariable *gv = cast<GlobalVariable>(subProgram);
    ConstantStruct *cs = cast<ConstantStruct>(gv->getInitializer());
    unsigned int line = unsigned(cast<ConstantInt>(cs->getOperand(7))->getZExtValue());
    Value *V = cast<Value>(cs->getOperand(6));
    GlobalVariable *gv2 = cast<GlobalVariable>(cast<ConstantExpr>(V)->getOperand(0));
    ConstantStruct *cs2 = cast<ConstantStruct>(gv2->getInitializer());
    std::string file, dir;
    llvm::GetConstantStringInfo(cs2->getOperand(3), file);
    llvm::GetConstantStringInfo(cs2->getOperand(4), dir);
    std::string name = F->getName();

    // add source context of this invariant to context file
    ContextManager::addSpectrumContext(
      spectrumIndex,     // spectrumIndex
      nComponents,       // componentIndex
      dir,               // path
      file,              // file
      line,              // line
      name);             // name

    // find insertion point
    BasicBlock *bb = F->begin();
    BasicBlock::iterator InsertPos = bb->begin();
    while (isa<AllocaInst>(InsertPos)) ++InsertPos;

    // add call to lib function
    std::vector<Value*> Args(2);
    Args[0] = ConstantInt::get(Type::Int32Ty, spectrumIndex);
    Args[1] = ConstantInt::get(Type::Int32Ty, nComponents++);
         
    CallInst::Create(SpFn, Args.begin(), Args.end(), "", InsertPos);
  }

  // add the registration of the instrumented spectrum points in the _registerAll() function
  addSpectrumRegistration(M, spectrumIndex, nComponents, "Functions");

  llvm::cerr << "instrument: " << nComponents << " functions instrumented\n";

  // notify change of program 
  return true;
}


