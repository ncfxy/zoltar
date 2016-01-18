#ifndef __OPERATOR_H
#define __OPERATOR_H

#include "llvm/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/BasicBlock.h"

#include <string>
#include <map>
#include <list>

using namespace llvm;

class MutationOperator {
  public:
    //needs static bool isCompatible(const Instruction *I);
    virtual Value* apply(BasicBlock::iterator &I) = 0;
};

class OperatorInfo {
  
  ///
  /// The type of the constructor function for any Operator.
  ///
  /// Actually, the type corresponds to the callScreenerCtor() template, that is
  /// used to indirectly call the constructor.
public:
  typedef MutationOperator* (*OperatorCtor_t)();
  
  ///
  /// The type of the instruction compatibility check for any Operator.
  typedef bool (*CheckCompatible_t)(BasicBlock::iterator&);
  
public:
  ///
  /// Name of the operator in the registry.
  const std::string operatorName;
  
  const std::string operatorDescription;

private:
  bool              enabled;
  OperatorCtor_t    constructorFun;
  CheckCompatible_t checkFun;

public:
  OperatorInfo(const std::string& name, const std::string& descr, OperatorCtor_t ctor, CheckCompatible_t check);

  ///
  /// Name of the Operator.
  ///
  /// @return The name of the screener.
  const std::string& getOperatorName() const {
    return operatorName;
  }
  
  const std::string& getOperatorDescription() const {
    return operatorDescription;
  }
    
  /// Is the Screener enabled?
  bool isEnabled() {
    return enabled;
  }

  ///
  /// Enable or disale the operator.
  void setEnabled(bool en) {
    enabled = en;
  }

  bool isCompatible(BasicBlock::iterator &I) {
    return checkFun(I);
  }
  
  MutationOperator *build() const {
    return constructorFun();
  }

};

///
/// Helper type. A std::map of OperatorInfo entries mapped by id.
typedef std::map<int,OperatorInfo*> OperatorInfoTable;

///
/// Helper type. A std::map of OperatorInfo entries mapped by name.
typedef std::map<const std::string,OperatorInfo*> OperatorInfoNameTable;

///
/// Helper type. A std::list of OperatorInfo entries.
typedef std::list<OperatorInfo*> OperatorInfoList;

typedef std::list<std::string> OperatorNameList;

///
/// Singleton class that stores all the registered operators.
class OperatorManager {

  static OperatorManager instance;
  static unsigned nextId;
  
public:
  
  ///
  /// Fetch the singleton instance.
  ///
  /// @return The singleton.
  static OperatorManager* getInstance();
	
  ///
  /// Register a new OperatorInfo entry.
  ///
  /// @param oi The new OperatorInfo entry. It won't be deleted on destruction,
  /// It's your responsibility.
  unsigned registerOperator(OperatorInfo& oi);
	
  ///
  /// Find compatible operators.
  ///
  /// Places a reference of each compatible Screener into the list parameter.
  ///
  /// @param tag
  /// @param type
  /// @param list A list where resutls will be stored.
  void getCompatibleOperators(BasicBlock::iterator &BI, OperatorInfoList& list);
  
  OperatorNameList& getRegisteredNames() {
    return operatorNames;
  }
  
  OperatorInfoList& getRegistered() {
    return operatorList;
  }
  
  const OperatorInfo* getByName(const std::string& name) const;
  
  void enableByName(const std::string&);
  
private:
	
  ///
  /// Internal screener table by id.
  OperatorInfoTable operators;
  OperatorInfoList  operatorList;
  
  ///
  /// Internal screener table by name.
  OperatorInfoNameTable operatorsByName;
  
  ///
  /// Screener name list.
  OperatorNameList operatorNames;
  
protected:
  
  ///
  /// Internal constructor, shouldn't be called.
  OperatorManager() : operators(), operatorList(), operatorsByName(), operatorNames() {}  
};

///
/// Call the constructor of OperatorName, which must be of type Operator.
///
/// @param msg TraceMessage to use for initialisation.
template<typename OperatorName>
MutationOperator *callOperatorCtor() { return new OperatorName(); }

///
/// Check wether OperatorName is compatible with Instruction I.
///
template<typename OperatorName>
bool callCheckCompatible(BasicBlock::iterator &BI) { return OperatorName::isCompatible(BI); }


///
/// Magical template that puts a Operator into a ScreenerInfo and the registers 
/// it into the OperatorManager registry.
///
/// You should use this template to register your new screeners like this in
/// your .cpp
///
/// \code
/// static RegisterOperator<NameOfYourOperator> dummyvar("name");
/// \endcode
///
template<typename OperatorName>
struct RegisterOperator : public OperatorInfo {
  
  ///
  /// Register the Screener with a name.
  ///
  /// @param name Desired name of the Operator.
  RegisterOperator(const char *name, const char *descr="")
	: OperatorInfo(name, descr,
                   OperatorInfo::OperatorCtor_t(callOperatorCtor<OperatorName>), 
                   OperatorInfo::CheckCompatible_t(callCheckCompatible<OperatorName>)) {
	  OperatorName::ID = OperatorManager::getInstance()->registerOperator(*this);
	}
	
};

class OperatorNameParser : public cl::parser<OperatorInfo*> {
  cl::Option *Opt;
public:
  OperatorNameParser() : Opt(0) {}

  void initialize(cl::Option &O) {
    Opt = &O;
    cl::parser<OperatorInfo*>::initialize(O);

    // Add all of the passes to the map that got initialized before 'this' did.
    //enumeratePasses();
    OperatorInfoList& OpIs = OperatorManager::getInstance()->getRegistered();
    
    for (OperatorInfoList::iterator opi = OpIs.begin(); opi != OpIs.end(); opi++) {
      std::string name = (*opi)->getOperatorName();
      addLiteralOption(name.c_str(), *opi, (*opi)->getOperatorDescription().c_str());
    }
  }


  // ValLessThan - Provide a sorting comparator for Values elements...
  typedef std::pair<const char*,
                    std::pair<OperatorInfo*, const char*> > ValType;
  static bool ValLessThan(const ValType &VT1, const ValType &VT2) {
    return std::string(VT1.first) < std::string(VT2.first);
  }

  // printOptionInfo - Print out information about this option.  Override the
  // default implementation to sort the table before we print...
  virtual void printOptionInfo(const cl::Option &O, size_t GlobalWidth) const {
    OperatorNameParser *PNP = const_cast<OperatorNameParser*>(this);
    std::sort(PNP->Values.begin(), PNP->Values.end(), ValLessThan);
    cl::parser<OperatorInfo*>::printOptionInfo(O, GlobalWidth);
  }
};


#endif
