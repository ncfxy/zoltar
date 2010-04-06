
namespace llvm {
  class IndexManager {
    unsigned int nSpectra;
    unsigned int nInvariantTypes;
    static IndexManager *indexManager;
    IndexManager();
  public:
    static unsigned int getSpectrumIndex();
    static unsigned int getInvariantTypeIndex();
  };
}

