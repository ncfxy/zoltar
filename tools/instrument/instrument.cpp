//===- opt.cpp - The LLVM Modular Optimizer -------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Optimizations may be specified an arbitrary number of times on the command
// line, They are run in the order specified.
//
//===----------------------------------------------------------------------===//

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
//#include "llvm/ModuleProvider.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Analysis/Verifier.h"

#include "llvm/Target/TargetData.h"
#include "llvm/Support/PassNameParser.h"
#include "llvm/System/Signals.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"

#include "llvm/Support/SystemUtils.h"


#include "contextManager.h"

#include "Operator.h"

#include <cstring>

using namespace llvm;

namespace {
  
  class ZoltarPass {
    public:
    bool operator()(const PassInfo &P) const {

      const char * n = P.getPassArgument();
      if (strstr(n, "inv") == n)
        return true;
      if (strstr(n, "sp") == n)
        return true;
      if (strcmp(n, "memprotection") == 0)
        return true;
      if (strcmp(n, "bypassmain") == 0)
        return true;
      if (strstr(n, "mutation") == n)
        return true;
      return false;
    }
  };

}

// The OptimizationList is automatically populated with registered Passes by the
// PassNameParser.
//
static cl::list<const PassInfo*, bool, FilteredPassNameParser<ZoltarPass> >
PassList(cl::desc("Instrumentations available:"));

// The MutationsList is automatically populated with registered Operators by the
// OperatorNameParser.
//
static cl::list<OperatorInfo*, bool, OperatorNameParser>
OperatorList(cl::desc("Mutation operators available:"));

static cl::opt<bool>
MutOps("mutops", cl::desc("Specify mutation operators one by one"));

// Other command line options...
//
static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input bitcode file>"),
    cl::init("-"), cl::value_desc("filename"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Override output filename"),
               cl::value_desc("filename"), cl::init("-"));

static cl::opt<bool>
Force("f", cl::desc("Overwrite output files"));

static cl::opt<bool>
NoOutput("disable-output",
         cl::desc("Do not write result bitcode file"), cl::Hidden);

static cl::opt<bool>
Quiet("quiet", cl::desc("Obsolete option"), cl::Hidden);

//===----------------------------------------------------------------------===//
// main for instrument
//
int main(int argc, char **argv) {
  llvm_shutdown_obj X;  // Call llvm_shutdown() on exit.
  LLVMContext &Context = getGlobalContext();
  try {
    cl::ParseCommandLineOptions(argc, argv,
      "zoltar .bc -> .bc instrumenter and mutator\n");
    sys::PrintStackTraceOnErrorSignal();

    // Allocate a full target machine description only if necessary.
    // FIXME: The choice of target should be controllable on the command line.
    std::auto_ptr<TargetMachine> target;

    std::string ErrorMessage;

    // Load the input module...
    std::auto_ptr<Module> M;
    if (MemoryBuffer *Buffer
          = MemoryBuffer::getFileOrSTDIN(InputFilename, &ErrorMessage)) {
      M.reset(ParseBitcodeFile(Buffer, Context, &ErrorMessage));
      delete Buffer;
    }

    if (M.get() == 0) {
      errs() << argv[0] << ": ";
      if (ErrorMessage.size())
        errs() << ErrorMessage << "\n";
      else
        errs() << "bitcode didn't read correctly.\n";
      return 1;
    }

    // Figure out what stream we are supposed to write to...
    // FIXME: outs() is not binary!
    raw_ostream *Out = &outs();  // Default to printing to stdout...
    if (OutputFilename != "-") {
      std::string ErrorInfo;
      /*TODO: solve this problem */
      //Out = new raw_fd_ostream(OutputFilename.c_str(), /*Binary=*/true,
       //                        Force, ErrorInfo);
      Out = new raw_fd_ostream(OutputFilename.c_str(),ErrorInfo,0);
      if (!ErrorInfo.empty()) {
        errs() << ErrorInfo << '\n';
        if (!Force)
          errs() << "Use -f command line argument to force output\n";
        delete Out;
        return 1;
      }

      // Make sure that the Output file gets unlinked from the disk if we get a
      // SIGINT
      sys::RemoveFileOnSignal(sys::Path(OutputFilename));
    }

    // If the output is set to be emitted to standard out, and standard out is a
    // console, print out a warning message and refuse to do it.  We don't
    // impress anyone by spewing tons of binary goo to a terminal.
    if (!Force && !NoOutput && CheckBitcodeOutputToConsole(*Out,!Quiet)) {
      NoOutput = true;
    }

    // Create a PassManager to hold and optimize the collection of passes we are
    // about to build...
    //
    PassManager Passes;

    // Add an appropriate TargetData instance for this module...
    Passes.add(new TargetData(M.get()));

    // Create a new instrumentation pass for each one specified on the command line
    for (unsigned i = 0; i < PassList.size(); ++i) {
      
      const PassInfo *PassInf = PassList[i];
      Pass *P = 0;
      if (PassInf->getNormalCtor())
        P = PassInf->getNormalCtor()();
      else
        errs() << argv[0] << ": cannot create pass: "
               << PassInf->getPassName() << "\n";
      if (P) {
        Passes.add(P);
      }
    }
    
    // Enable the specified mutation operators
    if (!MutOps) {
      OperatorManager* OM = OperatorManager::getInstance();
      OperatorInfoList::iterator oit;
      for (oit = OM->getRegistered().begin(); oit != OM->getRegistered().end(); oit++) {
        (*oit)->setEnabled(true);
      }
    } else {    
        for (unsigned i = 0; i < OperatorList.size(); ++i) {
          
          OperatorInfo *OInf = OperatorList[i];
          
          OInf->setEnabled(true);
        }
    }
    //Passes.add(createPrintModulePass(&errs()));
    // Check that the module is well formed on completion of optimization
    Passes.add(createVerifierPass());

    // Write bitcode out to disk or outs() as the last step...
    if (!NoOutput)
      Passes.add(createBitcodeWriterPass(*Out));

    // Now that we have all of the passes ready, run them.
    Passes.run(*M.get());

    // Delete the raw_fd_ostream.
    if (Out != &outs())
      delete Out;
    
    // Write the context.dat file of zoltar
    ContextManager::print();

    return 0;

  } catch (const std::string& msg) {
    errs() << argv[0] << ": " << msg << "\n";
  } catch (...) {
    errs() << argv[0] << ": Unexpected unknown exception occurred.\n";
  }
  llvm_shutdown();
  return 1;
}
