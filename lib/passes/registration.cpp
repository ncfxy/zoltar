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

  RegFn = cast<Function>(M.getOrInsertFunction("_registerAll", Type::VoidTy, NULL));
  BasicBlock *entry = BasicBlock::Create("entry", RegFn);
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
  // add global string to hold invariant type name
  Constant *invariantTypeNameC = ConstantArray::get(invariantTypeName);
  GlobalValue *invariantTypeNameGV = new GlobalVariable(invariantTypeNameC->getType(), 
                                                  true, 
                                                  GlobalValue::InternalLinkage,
                                                  invariantTypeNameC,
                                                  "invarianttypename",
                                                  &M);

  Function *RegFn = createRegisterAllFunction(M);
    
  // add instrumentation registration call to _registerAll function
  Constant *InitFn = M.getOrInsertFunction("_registerInvariantType", 
                          Type::VoidTy, 
                          Type::Int32Ty,  // timestamp
                          Type::Int32Ty,  // invarianttype index
                          Type::Int32Ty,  // nInvariants
                          Type::Int32Ty,  // name
                          Type::Int32Ty,  // isTimerUpdated
                          NULL);

  BasicBlock *Entry = RegFn->begin();
  BasicBlock::iterator InsertPos = Entry->begin();
  while (isa<AllocaInst>(InsertPos)) ++InsertPos;
    
  std::vector<Value*> Args(5);
  Args[0] = ConstantInt::get(Type::Int32Ty, (unsigned int) time(0));
  Args[1] = ConstantInt::get(Type::Int32Ty, invariantTypeIndex);
  Args[2] = ConstantInt::get(Type::Int32Ty, nInvariants);
  Args[3] = CastInst::createPointerCast(invariantTypeNameGV, Type::Int32Ty, "st.cast", InsertPos);
  Args[4] = ConstantInt::get(Type::Int32Ty, isTimerUpdated);

  CallInst::Create(InitFn, Args.begin(), Args.end(), "", InsertPos);
}

/* Adds the registration call of the instrumented points for generating spectrum data
   to the _registerAll() function */
void llvm::addSpectrumRegistration(Module &M,
                                  int spectrumIndex, 
                                  int nComponents, 
                                  const std::string &spectrumName) {
  // add global string to hold spectrum name
  Constant *spectrumNameC = ConstantArray::get(spectrumName);
  GlobalValue *spectrumNameGV = new GlobalVariable(spectrumNameC->getType(), 
                                                  true, 
                                                  GlobalValue::InternalLinkage,
                                                  spectrumNameC,
                                                  "spectrumname",
                                                  &M);

  Function *RegFn = createRegisterAllFunction(M);
    
  // add instrumentation registration call to _registerAll function
  Constant *InitFn = M.getOrInsertFunction("_registerSpectrum", 
                          Type::VoidTy, 
                          Type::Int32Ty,  // timestamp
                          Type::Int32Ty,  // spectrum index
                          Type::Int32Ty,  // nComponents
                          Type::Int32Ty,  // name
                          NULL);

  BasicBlock *Entry = RegFn->begin();
  BasicBlock::iterator InsertPos = Entry->begin();
  while (isa<AllocaInst>(InsertPos)) ++InsertPos;
    
  std::vector<Value*> Args(4);
  Args[0] = ConstantInt::get(Type::Int32Ty, (unsigned int) time(0));
  Args[1] = ConstantInt::get(Type::Int32Ty, spectrumIndex);
  Args[2] = ConstantInt::get(Type::Int32Ty, nComponents);
  Args[3] = CastInst::createPointerCast(spectrumNameGV, Type::Int32Ty, "st.cast", InsertPos);

  CallInst::Create(InitFn, Args.begin(), Args.end(), "", InsertPos);
}
