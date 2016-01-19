#include "Operator.h"
#include "llvm/Instruction.h"

#include <iostream>
#include <map>

OperatorInfo::OperatorInfo(const std::string& name, const std::string& descr, OperatorCtor_t ctor, CheckCompatible_t check)
: operatorName(name), operatorDescription(descr), enabled(false), constructorFun(ctor), checkFun(check) {
}

OperatorManager* OperatorManager::instance;
unsigned OperatorManager::nextId = 0;


OperatorManager* OperatorManager::getInstance() {
	if(instance == NULL){
		instance = new OperatorManager();
		return instance;
	}
    return instance;
}


unsigned OperatorManager::registerOperator(OperatorInfo& oi) {
  std::cout << nextId << " " << oi.operatorName << "\n";
  operators.insert(std::pair<int, OperatorInfo *>(nextId, &oi));

  operatorsByName[oi.operatorName] = &oi;
  operatorNames.push_back(oi.operatorName);
  operatorList.push_back(&oi);
  return nextId++;
}

void OperatorManager::getCompatibleOperators(BasicBlock::iterator &I, OperatorInfoList& list) {
  list.clear();
  OperatorInfoTable::iterator it;
  for (it = operators.begin(); it != operators.end(); it++) {
    if (it->second->isCompatible(I) && it->second->isEnabled() ) {
      list.push_back(it->second);
    }
  }
}

const OperatorInfo* OperatorManager::getByName(const std::string& name) const {
  OperatorInfoNameTable::const_iterator res;
  if ((res = operatorsByName.find(name)) != operatorsByName.end()) {
    return res->second;
  }
  return 0;
}


void OperatorManager::enableByName(const std::string& name) {
  OperatorInfo* in = const_cast<OperatorInfo*>(getByName(name));
  if (in != 0) {
    in->setEnabled(true);
  }
}


