
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Analysis/ValueTracking.h"

#include <string>
#include <sstream>


using namespace llvm;

class LocationInfo {

public:
  std::string dir;
  std::string file;
  std::string fname;
  int line;
  int bb;
  int ff;
  int inst_offset;
  
  LocationInfo() {
    dir = "-";
    file = "-";
    fname = "-";
    line = 0;
    bb = -1;
    ff = -1;
    F = NULL;
    BB = NULL;
  }
  
  void update(Instruction &I) {
    
    BasicBlock *NBB = I.getParent();
    Function   *NF  = NBB->getParent();
    
    if (NF != this->F) {
      this->F = NF;
      this->fname = NF->getNameStr();
      ff++;
      inst_offset = -1;
    }
    
    if (NBB != this->BB) {
      this->BB = NBB;
      bb++;
    }

    inst_offset++;
    /*TODO: solve DbgStopPointInst problem*/
    /*if(isa<DbgStopPointInst>(I)) {
      DbgStopPointInst &DSPI = cast<DbgStopPointInst>(I);
      
      this->dir.clear(); this->file.clear();
      llvm::GetConstantStringInfo(DSPI.getDirectory(), this->dir);
      llvm::GetConstantStringInfo(DSPI.getFileName(), this->file);
      this->line = DSPI.getLine();
    }*/
  }
  
  std::string getId() {
    
    std::ostringstream oss;
    
    oss << dir << file << ":" << line << ":" << fname;
    oss << "+" << inst_offset;
    
    return oss.str();
  }
  
private:
  Function   *F;
  BasicBlock *BB;
};

bool
isAccumulator(Instruction *I);

bool
isIncrementOrDecrement(Instruction *I);

Value *
getOldValue(Instruction *I);
