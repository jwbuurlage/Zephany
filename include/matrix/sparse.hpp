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

        // should create the stream for A
        // modify the stream by filling it with strips and windows
        // need local variables and *vector distribution*
        // for now simple implementation as proof of concept. Fixed
        // 'window/strip size' and just put stuff in blocks

        // preliminary computation: num strips, num windows
        // 'block distribution'
        constexpr int stripSize = stream_.windowDimension;    // strip size in columns
        constexpr int windowHeight = stream_.windowDimension; // windowHeight in rows

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

        for (auto image : this->getImages()) {
            for (auto triplet : *image) {
                // we dont want a partitioned matrix here, just loop over
                // triplets
                // FIXME: look at matrix market, possibly generalize
                // triplet.row(), triplet.col(), triplet.value()
                //
                // compute block number of triplet
                auto strip = triplet.col() / stripSize;
                auto windowInStrip = triplet.row() / windowHeight;
                // compute target processor
                // update arrays
            }
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
