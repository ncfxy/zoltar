#include <string>
#include <vector>
#include <map>

namespace llvm {
  struct PathEntry {
    int index;
    std::string path;
  };
  struct FileEntry {
    int index;
    std::string file;
  };
  struct Context {
    int index;
    int subindex;
    int pathIndex;
    int fileIndex;
    int line;
    std::string varName;
  };
  class ContextManager {
    static ContextManager *contextManager;

    std::map<std::string, int> paths;
    std::map<std::string, int> files;
    std::vector<Context> spectraContexts;
    std::vector<Context> invariantTypesContexts;

    ContextManager();

  public:
    static void addSpectrumContext(int spectrumIndex, int componentIndex, std::string path, std::string file, int line, std::string varName);
    static void addInvariantTypeContext(int invariantTypeIndex, int invariantIndex, std::string path, std::string file, int line, std::string varName);
    static void print();
  };
}

