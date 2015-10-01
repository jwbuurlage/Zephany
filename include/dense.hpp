#include <zee.hpp>

#include "streams.hpp"

namespace Zephany {

using namespace Zee;

template <typename TVal = default_scalar_type,
          typename TIdx = default_index_type>
class DStreamingMatrix
    : public DDenseMatrixBase<DStreamingMatrix<TVal, TIdx>, TVal, TIdx> {
  public:
    using Base = DDenseMatrixBase<DStreamingMatrix<TVal, TIdx>, TVal, TIdx>;
    using Base::operator=;

    DStreamingMatrix(TIdx rows, TIdx cols)
        : Base(rows, cols), stream_(stream_direction::down) {
        resize(rows, cols);
        initializeStream();
    }

    explicit DStreamingMatrix(std::string file)
        : Base(0, 0), stream_(stream_direction::down) {
        matrix_market::load(file, *this);
        initializeStream();
    }

    explicit DStreamingMatrix(const Stream<TVal>& upStream) {
        ZeeAssert(upStream.direction == stream_direction::up);
        initializeStream();
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
    //--------------------------------------------------------------------------

    // Look into partial updates?
    void updateStream() {
        stream_.feedElements(elements_);
    }

  private:
    void initializeStream() {
        constexpr int innerBlocks = stream_config::N;
        constexpr int innerBlockSize = 8;
        const int outerBlockSize = innerBlocks * innerBlockSize;
        const int outerBlocks =
            this->getRows() / outerBlockSize;
        ZeeAssert(this->getRows() % outerBlockSize == 0);

        stream_.setInner(innerBlocks, innerBlockSize);
        stream_.setOuter(outerBlocks, outerBlockSize);
        stream_.setMatrixSize(this->getRows());
        stream_.computeChunkSize();
    }

    // stored column major
    std::vector<std::vector<TVal>> elements_;
    MatrixBlockStream<TVal> stream_;
};

} // namespace Zephany
