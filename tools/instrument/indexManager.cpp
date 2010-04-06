
#include <iostream>

#include "indexManager.h"

using namespace llvm;

  IndexManager *IndexManager::indexManager = 0;
  IndexManager::IndexManager() {
    nSpectra = 0;
    nInvariantTypes = 0;
  }
  unsigned int IndexManager::getSpectrumIndex() {
    if(!indexManager) {
      indexManager = new IndexManager();
    } 
    unsigned int res = indexManager->nSpectra;
    indexManager->nSpectra++;
    return res;
  }
  unsigned int IndexManager::getInvariantTypeIndex() {
    if(!indexManager) {
      indexManager = new IndexManager();
    }
    unsigned int res = indexManager->nInvariantTypes;
    indexManager->nInvariantTypes++;
    return res;
  }


