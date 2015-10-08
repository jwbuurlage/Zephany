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

    void prepareStream() {
        ZeeAssert(this->getRows() > stream_.windowDimension &&
                  this->getCols() > stream_.windowDimension);

        ZeeLogVar(this->getRows());
        ZeeLogVar(this->getCols());
        ZeeLogVar(this->nonZeros());

        // should create the stream for A
        // modify the stream by filling it with strips and windows
        // need local variables and *vector distribution*
        // for now simple implementation as proof of concept. Fixed
        // 'window/strip size' and just put stuff in blocks

        // preliminary computation: num strips, num windows
        // 'block distribution'
        constexpr int stripSize = stream_.windowDimension;    // strip size in columns
        constexpr int windowHeight = stream_.windowDimension; // windowHeight in rows

        int strips = (this->getCols() - 1) / stripSize + 1;
        int windows = (this->getRows() - 1) / windowHeight + 1;

        // first scan: get and set:
        // > global headers:
        // max_u_size
        // max_v_size
        // max_block_size
        //
        // > window headers:
        // num_nonlocal
        // loc_nonlocal
        // idx_nonlocal
        // window_size

        using WindowInfo = std::array<std::vector<TIdx>, stream_config::processors>;
        WindowInfo localWindowSize;
        WindowInfo inputSize;
        WindowInfo outputSize;
        WindowInfo numNonLocalInput;
        // ...

        TIdx s = 0;
        for (auto image : this->getImages()) {
            for (auto triplet : *image) {
                // compute block number of triplet
                auto strip = triplet.col() / stripSize;
                auto window = triplet.row() / windowHeight;

                // update arrays
                localWindowSize[s][strip * windows + window]++;
                // FIXME after vector distribution..
            }
            ++s;
        }

        std::array<TIdx, stream_config::processors> maxOutputSize;
        std::array<TIdx, stream_config::processors> maxInputSize;
        std::array<TIdx, stream_config::processors> maxWindowSize;

        // second run: fill in windows
        for (auto image : this->getImages()) {
            for (auto triplet : *image) {
            }
        }

        streamInitialized_ = true;
    }

    // FIXME remove, needed by MG
    bool isInitialized() const { return true; }

    double loadImbalance() const override { return -1.0; }
    TIdx communicationVolume() const override { return 0; }

  private:
    SparseStrippedStream<TVal, TIdx> stream_;
    bool streamInitialized_ = false;
};

} // namespace zephany
