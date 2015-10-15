#pragma once

#include "streams.hpp"

namespace Zephany {

template <typename TMatrix, typename TVector>
class SparseStream: Stream<typename TMatrix::value_type, typename TMatrix::index_type> {
  public:
    using TVal = typename TMatrix::value_type;
    using TIdx = typename TMatrix::index_type;

    SparseStream(TMatrix& A, TVector& v, TIdx stripSize, TIdx windowSize)
        : Stream<TVal, TIdx>(stream_direction::down), A_(A), v_(v),
          stripSize_(stripSize), windowSize_(windowSize) {}

    void create() const override {
        // TODO implement
        ZeeLogDebug << "SparseStream::create() not implemented" << endLog;
    }

    void prepareStream() {
        ZeeAssert(A_.getRows() > windowSize_ &&
                  A_.getCols() > stripSize_);

        ZeeLogVar(A_.getRows());
        ZeeLogVar(A_.getCols());
        ZeeLogVar(A_.nonZeros());

        ZeeAssert(A_.getProcs() == stream_config::processors);

        int strips = (A_.getCols() - 1) / stripSize_ + 1;
        int windows = (A_.getRows() - 1) / windowSize_ + 1;

        std::array<std::vector<std::vector<Triplet<TVal, TIdx>>>,
                       stream_config::processors> windowTriplets;

        for (auto& windowList : windowTriplets) {
            windowList.resize(strips * windows);
        }

        TIdx s = 0;
        for (auto image : A_.getImages()) {
            for (auto triplet : *image) {
                // compute block number of triplet
                auto strip = triplet.col() / stripSize_;
                auto window = triplet.row() / windowSize_;
                auto windowIdx = strip * windows + window;

                windowTriplets[s][windowIdx].push_back(triplet);
            }
            ++s;
        }

        // now we have a list of windows
    }

  private:
    TMatrix& A_;
    TVector& v_;

    TIdx stripSize_;
    TIdx windowSize_;
};

} // namespace Zephany
