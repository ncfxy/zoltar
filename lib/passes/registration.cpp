#include "llvm/Constants.h"
#include "llvm/ValueSymbolTable.h"
#include "llvm/Value.h"
#include "llvm/Module.h"
#include "llvm/Support/IRBuilder.h"
#include <time.h>
#include <iostream>

#include "registration.h"

using namespace llvm;

/* creates _registerAll() function if it is not yet created
   the _registerAll() function is inserted in the instrumented program
   and will initialize all instrumented spectrum and invariant data */
Function* llvm::createRegisterAllFunction(Module &M) {
  Function *RegFn;
  if((RegFn = M.getFunction("_registerAll")) != NULL)
    return RegFn;

  RegFn = cast<Function>(M.getOrInsertFunction("_registerAll", Type::getVoidTy(M.getContext()), NULL));
  BasicBlock *entry = BasicBlock::Create(M.getContext(), "entry", RegFn);
  IRBuilder<true> builder(entry);
  builder.CreateRetVoid();

  std::cerr << "instrument: _registerAll() function inserted\n";

  return RegFn;
}

/* Adds the registration call of the instrumented invariant type
   to the _registerAll() function */
void llvm::addInvariantTypeRegistration(Module &M,
                                  int invariantTypeIndex, 
                                  int nInvariants, 
                                  const std::string &invariantTypeName, 
                                  int isTimerUpdated) {
  LLVMContext &C = M.getContext();
  
  // add global string to hold invariant type name
  Constant *invariantTypeNameC = ConstantArray::get(C, invariantTypeName);
  GlobalValue *invariantTypeNameGV = new GlobalVariable(M,
                                                  invariantTypeNameC->getType(), 
                                                  true, 
                                                  GlobalValue::InternalLinkage,
                                                  invariantTypeNameC,
                                                  "invarianttypename");

  Function *RegFn = createRegisterAllFunction(M);
    
  // add instrumentation registration call to _registerAll function
  Constant *InitFn = M.getOrInsertFunction("_registerInvariantType", 
                          Type::getVoidTy(C), 
                          Type::getInt32Ty(C),  // timestamp
                          Type::getInt32Ty(C),  // invarianttype index
                          Type::getInt32Ty(C),  // nInvariants
                          Type::getInt32Ty(C),  // name
                          Type::getInt32Ty(C),  // isTimerUpdated
                          NULL);

  BasicBlock *Entry = RegFn->begin();
  BasicBlock::iterator InsertPos = Entry->begin();
  while (isa<AllocaInst>(InsertPos)) ++InsertPos;
    
  std::vector<Value*> Args(5);
  Args[0] = ConstantInt::get(Type::getInt32Ty(C), (unsigned int) time(0));
  Args[1] = ConstantInt::get(Type::getInt32Ty(C), invariantTypeIndex);
  Args[2] = ConstantInt::get(Type::getInt32Ty(C), nInvariants);
  Args[3] = CastInst::CreatePointerCast(invariantTypeNameGV, Type::getInt32Ty(C), "st.cast", InsertPos);
  Args[4] = ConstantInt::get(Type::getInt32Ty(C), isTimerUpdated);

  CallInst::Create(InitFn, Args.begin(), Args.end(), "", InsertPos);
}

/* Adds the registration call of the instrumented points for generating spectrum data
   to the _registerAll() function */
void llvm::addSpectrumRegistration(Module &M,
                                  int spectrumIndex, 
                                  int nComponents, 
                                  const std::string &spectrumName) {
  
  LLVMContext &C = M.getContext();
  
  // add global string to hold spectrum name
  Constant *spectrumNameC = ConstantArray::get(C, spectrumName);
  GlobalValue *spectrumNameGV = new GlobalVariable(M, 
                                                  spectrumNameC->getType(), 
                                                  true, 
                                                  GlobalValue::InternalLinkage,
                                                  spectrumNameC,
                                                  "spectrumname");

  Function *RegFn = createRegisterAllFunction(M);
    
  // add instrumentation registration call to _registerAll function
  Constant *InitFn = M.getOrInsertFunction("_registerSpectrum", 
                          Type::getVoidTy(C), 
                          Type::getInt32Ty(C),  // timestamp
                          Type::getInt32Ty(C),  // spectrum index
                          Type::getInt32Ty(C),  // nComponents
                          Type::getInt32Ty(C),  // name
                          NULL);

  BasicBlock *Entry = RegFn->begin();
  BasicBlock::iterator InsertPos = Entry->begin();
  while (isa<AllocaInst>(InsertPos)) ++InsertPos;
    
  std::vector<Value*> Args(4);
  Args[0] = ConstantInt::get(Type::getInt32Ty(C), (unsigned int) time(0));
  Args[1] = ConstantInt::get(Type::getInt32Ty(C), spectrumIndex);
  Args[2] = ConstantInt::get(Type::getInt32Ty(C), nComponents);
  Args[3] = CastInst::CreatePointerCast(spectrumNameGV, Type::getInt32Ty(C), "st.cast", InsertPos);

  CallInst::Create(InitFn, Args.begin(), Args.end(), "", InsertPos);
}
