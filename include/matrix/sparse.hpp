#include "../streams/sparse_stripped.hpp"

namespace Zephany {

using namespace Zee;

template <typename TVal, typename TIdx>
class DStreamingSparseMatrix
    : public DSparseMatrixBase<DStreamingSparseMatrix<TVal, TIdx>, TVal, TIdx> {
  public:
    using Base =
        DSparseMatrixBase<DStreamingSparseMatrix<TVal, TIdx>, TVal, TIdx>;

    DStreamingSparseMatrix(TIdx rows, TIdx cols) : Base(rows, cols) {}

    DStreamingSparseMatrix(std::string file, TIdx procs = 0)
        : Base(file, procs) {}

    // FIXME remove, needed by MG
    bool isInitialized() const { return true; }

    double loadImbalance() const override { return -1.0; }
    TIdx communicationVolume() const override { return 0; }

  private:
};

} // namespace zephany
