#include <zee.hpp>

#include "streams.hpp"

namespace Zephany {

using namespace Zee;

template <typename TVal = default_scalar_type,
          typename TIdx = default_index_type>
class DStreamingMatrix
    : public DDenseMatrixBase<DStreamingMatrix<TVal, TIdx>, TVal, TIdx> {
  public:
    static constexpr int innerBlocks = stream_config::N;
    static constexpr int innerBlockSize = 1;
    static constexpr int outerBlockSize = innerBlocks * innerBlockSize;

    using Base = DDenseMatrixBase<DStreamingMatrix<TVal, TIdx>, TVal, TIdx>;
    using Base::operator=;

    DStreamingMatrix(TIdx rows, TIdx cols)
        : Base(rows, cols), stream_(stream_direction::down) {
        resize(rows, cols);
        initializeStream_();
    }

    explicit DStreamingMatrix(std::string file)
        : Base(0, 0), stream_(stream_direction::down) {
        matrix_market::load(file, *this);
        initializeStream_();
    }

    explicit DStreamingMatrix(TIdx rows, TIdx cols,
                              const UpStream<TVal>& upStream)
        : Base(rows, cols), stream_(stream_direction::down) {
        resize(rows, cols);
        matrixFromUpStream_(upStream);
        initializeStream_();
    }

    MatrixBlockStream<TVal>& getStream() { return stream_; }
    const MatrixBlockStream<TVal>& getStream() const { return stream_; }

    //--------------------------------------------------------------------------
    // FIXME: this repeats DMatrix, do we actually want different storage here??
    // maybe inherit from DMatrix after all
    void resize(TIdx rows, TIdx cols) override {
        Base::resize(rows, cols);

        elements_.resize(rows);
        for (auto& row : elements_) {
            row.resize(cols);
        }
    }

    TVal& at(TIdx i, TIdx j) {
        ZeeAssert(i < this->rows_);
        ZeeAssert(j < this->cols_);
        return elements_[i][j];
    }

    const TVal& at(TIdx i, TIdx j) const {
        ZeeAssert(i < this->rows_);
        ZeeAssert(j < this->cols_);
        return elements_[i][j];
    }

    //--------------------------------------------------------------------------

    // TODO: Look into partial updates?
    void updateStream() {
        stream_.feedElements(elements_);
    }

  private:
    void initializeStream_() {
        const int outerBlocks = this->getRows() / outerBlockSize;
        ZeeAssert(this->getRows() % outerBlockSize == 0);

        stream_.setInner(innerBlocks, innerBlockSize);
        stream_.setOuter(outerBlocks, outerBlockSize);
        stream_.setMatrixSize(this->getRows());
        stream_.computeChunkSize();
    }

    void matrixFromUpStream_(const UpStream<TVal>& stream) {
        const int outerBlocks = this->getRows() / outerBlockSize;
        ZeeAssert(this->getRows() % outerBlockSize == 0);

        auto data = stream.getRawData();

        // This is always laid out left-handed (row major)
        for (int blockI = 0; blockI < outerBlocks; ++blockI)
        for (int blockJ = 0; blockJ < outerBlocks; ++blockJ) {
            int blockOffsetI = blockI * outerBlockSize;
            int blockOffsetJ = blockJ * outerBlockSize;
            for (int s = 0; s < stream_config::N; ++s)
            for (int t = 0; t < stream_config::N; ++t) {
                int procOffsetI = s * innerBlockSize;
                int procOffsetJ = t * innerBlockSize;
                for (int i = 0; i < innerBlockSize; ++i)
                for (int j = 0; j < innerBlockSize; ++j) {
                    elements_[blockOffsetI + procOffsetI + i][blockOffsetJ +
                                                              procOffsetJ + j] =
                        data[s * stream_config::N +
                             t][(blockI * outerBlocks + blockJ) *
                                    (innerBlockSize * innerBlockSize) +
                                i * innerBlockSize + j];
//                    ZeeLogInfo
//                        << "C[" << blockOffsetI + procOffsetI + i << ", "
//                        << blockOffsetJ + procOffsetJ + j << "] = "
//                        << data[s * stream_config::N +
//                                t][(blockI * outerBlocks + blockJ) *
//                                       (innerBlockSize * innerBlockSize) +
//                                   i * innerBlockSize + j] << endLog;
                }
            }
        }
    }

    // stored column major
    std::vector<std::vector<TVal>> elements_;
    MatrixBlockStream<TVal> stream_;
};

// We add an operator such that we can log dense matrices
template <typename TVal, typename TIdx>
Zee::Logger& operator <<(Logger& lhs, const DStreamingMatrix<TVal, TIdx>& rhs) {
    lhs << "\n";
    for (TIdx i = 0; i < rhs.getRows(); ++i) {
        lhs << "|";
        auto sep = "";
        for (TIdx j = 0; j < rhs.getCols(); ++j) {
            lhs << std::fixed << std::setprecision(2) << sep << rhs.at(i, j);
            sep = ", ";
        }
        lhs << "|";
        if (i != rhs.getRows() - 1)
            lhs << "\n";
    }
    return lhs;
}

} // namespace Zephany
