
#include <iostream>
#include <fstream>

#include "contextManager.h"

using namespace llvm;

  ContextManager *ContextManager::contextManager = 0;
  ContextManager::ContextManager() {
  }
  void ContextManager::addSpectrumContext(int spectrumIndex, int componentIndex, std::string path, std::string file, int line, std::string varName) {
    if(!contextManager) {
      contextManager = new ContextManager();
    }
    if(*path.end()!='/') {
      path.append(1,'/');
    }
    if(contextManager->paths.count(path+file)==0) {
      contextManager->paths.insert(std::pair<std::string, int>(path+file, contextManager->paths.size()));
    }
    if(contextManager->files.count(file)==0) {
      contextManager->files.insert(std::pair<std::string, int>(file, contextManager->files.size()));
    }
    Context c;
    c.index = spectrumIndex;
    c.subindex = componentIndex;
    c.pathIndex = contextManager->paths[path+file];
    c.fileIndex = contextManager->files[file];
    c.line = line;
    c.varName = varName;
    contextManager->spectraContexts.push_back(c);
  }
  void ContextManager::addInvariantTypeContext(int invariantTypeIndex, int invariantIndex, std::string path, std::string file, int line, std::string varName) {
    if(!contextManager) {
      contextManager = new ContextManager();
    } 
    Context c;
    c.index = invariantTypeIndex;
    c.subindex = invariantIndex;
    c.pathIndex = 0;
    c.fileIndex = 0;
    c.line = line;
    c.varName = varName;
    contextManager->invariantTypesContexts.push_back(c);
  }
  void ContextManager::print() {
    std::ofstream contextOut;
    contextOut.open("context.dat", std::ios::out);
    if(contextManager) {
      contextOut << contextManager->paths.size() << "\n";
      std::map<std::string, int>::iterator pit = contextManager->paths.begin();
      while(pit != contextManager->paths.end()) {
        contextOut << pit->second << " " << pit->first << "\n";
        pit++;
      }
      contextOut << contextManager->files.size() << "\n";
      std::map<std::string, int>::iterator fit = contextManager->files.begin();
      while(fit != contextManager->files.end()) {
        contextOut << fit->second << " " << fit->first << "\n";
        fit++;
      }
      std::cerr << "spectra: ";
      std::cerr << contextManager->spectraContexts.size() << "\n";
      std::vector<Context>::iterator scit = contextManager->spectraContexts.begin();
      while(scit != contextManager->spectraContexts.end()) {
        contextOut << "0 " << scit->index << " " << scit->subindex << " " << scit->pathIndex << " " << scit->fileIndex << " " << scit->line << " " << scit->varName << "\n";
        scit++;
      }
      std::cerr << "invariants: ";
      std::cerr << contextManager->invariantTypesContexts.size() << "\n";
      std::vector<Context>::iterator itcit = contextManager->invariantTypesContexts.begin();
      while(itcit != contextManager->invariantTypesContexts.end()) {
        contextOut << "1 " << itcit->index << " " << itcit->subindex << " " << itcit->pathIndex << " " << itcit->fileIndex << " " << itcit->line << " " << itcit->varName << "\n";
        itcit++;
      }
    }
    contextOut.close();
  }


