#pragma once

#include "streams.hpp"

namespace Zephany {

template <typename TMatrix, typename TVector>
class SparseStream
    : Stream<typename TMatrix::value_type, typename TMatrix::index_type> {
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
        ZeeAssert(A_.getRows() > windowSize_ && A_.getCols() > stripSize_);

        ZeeLogVar(A_.getRows());
        ZeeLogVar(A_.getCols());
        ZeeLogVar(A_.nonZeros());

        ZeeAssert(A_.getProcs() == stream_config::processors);

        // TODO: "windows" and "strips" should be constructed in some
        // partitioner, stored in matrix itself?
        TIdx strips = (A_.getCols() - 1) / stripSize_ + 1;
        TIdx windows = (A_.getRows() - 1) / windowSize_ + 1;

        std::array<std::vector<std::vector<Triplet<TVal, TIdx>>>,
                   stream_config::processors> windowTriplets;

        std::array<std::vector<std::set<TIdx>>, stream_config::processors>
            windowRowset;

        std::array<std::vector<std::set<TIdx>>, stream_config::processors>
            windowColset;

        for (TIdx s = 0; s < stream_config::processors; ++s) {
            windowTriplets[s].resize(strips * windows);
            windowRowset[s].resize(strips * windows);
            windowColset[s].resize(strips * windows);
        }

        TIdx s = 0;
        for (const auto& image : A_.getImages()) {
            for (auto triplet : *image) {
                // compute block number of triplet
                auto strip = triplet.col() / stripSize_;
                auto window = triplet.row() / windowSize_;
                auto windowIdx = strip * windows + window;

                windowTriplets[s][windowIdx].push_back(triplet);
                windowRowset[s][windowIdx].insert(triplet.row());
                windowColset[s][windowIdx].insert(triplet.col());
            }
            ++s;
        }

        std::array<std::vector<std::vector<TVal>>, stream_config::processors>
            stripValuesV;
        std::array<std::vector<std::vector<TIdx>>, stream_config::processors>
            stripIndicesV;

        for (TIdx s = 0; s < stream_config::processors; ++s) {
            stripValuesV[s].resize(strips);
            stripIndicesV[s].resize(strips);
        }

        auto& owners = v_.getOwners();
        ZeeLogVar(owners);

        // for each strip need local V
        for (TIdx strip = 0; strip < strips; ++strip) {
            // local V and values
            for (TIdx column = strip * stripSize_;
                 column < A_.getCols() && column < (strip + 1) * stripSize_;
                 ++column) {
                stripValuesV[owners[column]][strip].push_back(v_[column]);
                stripIndicesV[owners[column]][strip].push_back(column);
            }
        }

        // now we have a list of windows and strips
        // we now need:
        using WindowInfo = std::array<TIdx, stream_config::processors>;
        WindowInfo maxSizeU;
        WindowInfo maxSizeV;
        WindowInfo maxSizeWindow;
        WindowInfo maxNonLocal;
        for (TIdx s = 0; s < stream_config::processors; ++s) {
            for (TIdx windowIdx = 0; windowIdx < strips * windows;
                 ++windowIdx) {
                TIdx sizeV = windowColset[s][windowIdx].size();
                TIdx sizeU = windowRowset[s][windowIdx].size();
                TIdx sizeWindow = windowTriplets[s][windowIdx].size();
                TIdx nonLocal =
                    stripIndicesV[s][windowIdx / windows].size() - sizeV;
                if (sizeV > maxSizeV[s])
                    maxSizeV[s] = sizeV;
                if (sizeU > maxSizeU[s])
                    maxSizeU[s] = sizeU;
                if (sizeWindow > maxSizeWindow[s])
                    maxSizeWindow[s] = sizeWindow;
                if (nonLocal > maxNonLocal[s])
                    maxNonLocal[s] = nonLocal;
            }
        }

        localToGlobalU_.resize(strips * windows);

        // localize strip indices
        for (TIdx s = 0; s < stream_config::processors; ++s) {
            for (TIdx strip = 0; strip < strips; ++strips) {
                // better to do this in one run.. here we localize the strip
                // indices using a local map
                std::map<TIdx, TIdx> stripLocalIndices;
                for (TIdx i = 0; i < stripIndicesV[s][strip].size(); ++i) {
                    stripLocalIndices[stripIndicesV[s][strip][i]] = i;
                }

                // now we have local indices for the stripSize, and we loop over
                // windows
                for (TIdx window = 0; window < windows; ++window) {
                    // we find the global -> local u indices (and should store them)
                    // storing them is ~10% of matrix size, which is huge
                }
            }
        }

        // localize window indices
        // NOTE: store mapping for u for every proc, otherwise we cannot gather

        // fill the streams
        ZeeLogDebug << "Finished constructing stream" << endLog;
    }

  private:
    TMatrix& A_;
    TVector& v_;

    TIdx stripSize_;
    TIdx windowSize_;

    std::vector<std::vector<TIdx>> localToGlobalU_;
};

} // namespace Zephany
