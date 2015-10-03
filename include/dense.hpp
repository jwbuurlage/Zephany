#include <zee.hpp>

#include "streams.hpp"

namespace Zephany {

using namespace Zee;

#ifndef ZEPHANY_DEFAULT_INNER_SIZE
#define ZEPHANY_DEFAULT_INNER_SIZE 32
#endif

template <typename TVal = default_scalar_type,
          typename TIdx = default_index_type>
class DStreamingMatrix
    : public DDenseMatrixBase<DStreamingMatrix<TVal, TIdx>, TVal, TIdx> {
  public:
    using Base = DDenseMatrixBase<DStreamingMatrix<TVal, TIdx>, TVal, TIdx>;
    using Base::operator=;

    DStreamingMatrix(TIdx size)
        : DStreamingMatrix(ZEPHANY_DEFAULT_INNER_SIZE, size) {}

    DStreamingMatrix(TIdx innerBlockSize, TIdx size)
        : Base(size, size), stream_(stream_direction::down),
          innerBlockSize_(innerBlockSize) {
          ZeeAssertMsg(size >= innerBlockSize_ * innerBlocks_,
                       "Streaming matrices have to be larger than MN x MN, "
                       "where N is the dimension of the processor mesh and M "
                       "is the inner block size.");

        initializeStream_();
    }

    explicit DStreamingMatrix(std::string file)
        : Base(0, 0), stream_(stream_direction::down) {

        // TODO implement, maybe set inner blocks here already, prior to loading
        ZeeAssert(false);

        // FIXME deadlock: size not known, or stream not initialized
        initializeStream_();
        matrix_market::load(file, *this);
    }

    explicit DStreamingMatrix(TIdx rows, TIdx cols,
                              const UpStream<TVal>& upStream)
        : Base(rows, cols), stream_(stream_direction::down) {
        initializeStream_();
        matrixFromUpStream_(upStream);
    }

    DStreamingMatrix(DStreamingMatrix& other) = default;

    MatrixBlockStream<TVal, TIdx>& getStream() { return stream_; }
    const MatrixBlockStream<TVal, TIdx>& getStream() const { return stream_; }

    TVal& at(TIdx i, TIdx j) {
        ZeeAssert(i < this->rows_);
        ZeeAssert(j < this->cols_);

        return stream_.element(i, j);
    }

    const TVal& at(TIdx i, TIdx j) const {
        ZeeAssert(i < this->rows_);
        ZeeAssert(j < this->cols_);

        return stream_.element(i, j);
    }

    void setInnerBlockSize(TIdx innerBlockSize) {
        innerBlockSize_ = innerBlockSize;
        outerBlockSize_ = innerBlocks_ * innerBlockSize;
        // FIXME: stream has to be reshaped
    }

    void fillWithUpStream(const UpStream<TVal>& stream) {
        matrixFromUpStream_(stream);
    }

  private:
    void initializeStream_() {
        // FIXME pad zeroes?
        const TIdx outerBlocks =
            (this->getRows() - 1) / outerBlockSize_ + 1;

        stream_.setInner(innerBlocks_, innerBlockSize_);
        stream_.setOuter(outerBlocks, outerBlockSize_);
        stream_.setMatrixSize(this->getRows());
        stream_.computeChunkSize();
        stream_.reshape();
    }

    void matrixFromUpStream_(const UpStream<TVal>& stream) {
        const TIdx outerBlocks =
            (this->getRows() - 1) / outerBlockSize_ + 1;

        auto data = stream.getRawData();

        // This is always laid out left-handed (row major)
        for (TIdx blockI = 0; blockI < outerBlocks; ++blockI)
        for (TIdx blockJ = 0; blockJ < outerBlocks; ++blockJ) {
            TIdx blockOffsetI = blockI * outerBlockSize_;
            TIdx blockOffsetJ = blockJ * outerBlockSize_;
            for (TIdx s = 0; s < stream_config::N; ++s)
            for (TIdx t = 0; t < stream_config::N; ++t) {
                TIdx procOffsetI = s * innerBlockSize_;
                TIdx procOffsetJ = t * innerBlockSize_;
                for (TIdx i = 0; i < innerBlockSize_; ++i)
                for (TIdx j = 0; j < innerBlockSize_; ++j) {
                    // TODO: optimize this for speed.. this is ridiculous
                    if (blockOffsetI + procOffsetI + i >= this->rows_ ||
                        blockOffsetJ + procOffsetJ + j >= this->cols_)
                        break;

                    this->at(blockOffsetI + procOffsetI + i,
                             blockOffsetJ + procOffsetJ + j) =
                        data[s * stream_config::N +
                             t][(blockI * outerBlocks + blockJ) *
                                    (innerBlockSize_ * innerBlockSize_) +
                                i * innerBlockSize_ + j];
                }
            }
        }
    }

    // this should only be the stream
    MatrixBlockStream<TVal, TIdx> stream_;

    TIdx innerBlocks_ = stream_config::N;
    TIdx innerBlockSize_ = ZEPHANY_DEFAULT_INNER_SIZE;
    TIdx outerBlockSize_ = innerBlocks_ * innerBlockSize_;
};

// We add an operator such that we can log dense matrices
template <typename TVal, typename TIdx>
Logger& operator<<(Logger& lhs, const DStreamingMatrix<TVal, TIdx>& rhs) {
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
