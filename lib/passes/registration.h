#ifndef _REGISTRATION_H
#define _REGISTRATION_H

namespace llvm {
  class Module;
  class Function;

  Function *createRegisterAllFunction(Module &M);
  void addInvariantTypeRegistration(Module &M,
                                  int invariantTypeIndex, 
                                  int nInvariants, 
                                  const std::string &invariantTypeName, 
                                  int isTimerUpdated);
  void addSpectrumRegistration(Module &M,
                                  int spectrumIndex, 
                                  int nComponents, 
                                  const std::string &SpectrumName);
}

#endif
