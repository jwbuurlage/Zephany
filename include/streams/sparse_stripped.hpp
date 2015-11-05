#pragma once

// TODO:
// [ ] 'raw' stream with variable chunk sizeInBytes
//   [ ] include header sizes
//   [ ] write header sizes
//   [ ] check create_down_stream_raw
// [ ] check if indices get constructed correctly
// [ ] create up stream
// [ ] gather partial results of u_j with correct indices
// [ ] to avoid copies maybe we move v into a separate stream
// [ ] what about upStream? also create
// [ ] values of v are not at all important, so maybe ignore these
//     and write them only at spmv

#include <host_bsp.h>
#include "streams.hpp"
#include "stdint.h"
#include "../matrix/sparse.hpp"

namespace Zephany {

template <typename TIdx>
struct SparseStreamHeader {
    TIdx maxSizeU;
    TIdx maxSizeV;
    TIdx maxWindowSize;
    TIdx maxNonLocal;
    TIdx numStrips;

    void write(void** address) const {
        // I think struct is guarenteed to be C-compatible such that
        // we could just copy *this here.
        TIdx* ptr = (TIdx*)*address;
        ptr[0] = maxSizeU;
        ptr[1] = maxSizeV;
        ptr[2] = maxWindowSize;
        ptr[3] = maxNonLocal;
        ptr[4] = numStrips;
        address += this->sizeInBytes();
    }

    constexpr unsigned int sizeInBytes() const { return sizeof(TIdx) * 5; }
};

template <typename TVal, typename TIdx>
struct SparseStreamStripHeader {
    TIdx numWindows;
    std::vector<TVal> v;

    void write(void** address) const {
        // write to address and add to address pointer
        TIdx* ptr = (TIdx*)*address;
        TIdx i = 0;
        ptr[i++] = numWindows;
        ptr[i++] = (TIdx)v.size();
        TVal* valPtr = (TVal*)&ptr[i];
        i = 0;
        for (auto val : v) {
            valPtr[i++] = val;
        }
        address += this->sizeInBytes();
    }

    unsigned int sizeInBytes() const {
        return sizeof(TIdx) * 2 + sizeof(TVal) * v.size();
    }
};

template <typename TVal, typename TIdx>
struct SparseStreamWindow {
    std::vector<TIdx> nonLocalOwners;
    std::vector<TIdx> nonLocalIndices;

    std::vector<Triplet<TVal, TIdx>> triplets;
    TIdx sizeU;

    void write(void** address) const {
        // write to address and add to address pointer
        TIdx* ptr = (TIdx*)*address;
        TIdx i = 0;
        ptr[i++] = (TIdx)nonLocalOwners.size();
        for (auto owner : nonLocalOwners)
            ptr[i++] = owner;

        for (auto indices : nonLocalIndices)
            ptr[i++] = indices;
        ptr[i++] = sizeU;
        ptr[i++] = (TIdx)triplets.size();
        for (auto& triplet : triplets)
            ptr[i++] = triplet.row();
        for (auto& triplet : triplets)
            ptr[i++] = triplet.col();
        TVal* valPtr = (TVal*)&ptr[i];
        i = 0;
        for (auto& triplet : triplets)
            valPtr[i++] = triplet.value();
        address += this->sizeInBytes();
    }

    unsigned int sizeInBytes() const {
        return sizeof(TIdx) *
                   (2 * triplets.size() + 2 * nonLocalOwners.size() + 1) +
               sizeof(TVal) * triplets.size();
    }
};

template <typename TMatrix, typename TVector>
class SparseStream
    : Stream<typename TMatrix::value_type, typename TMatrix::index_type> {
  public:
    using TVal = typename TMatrix::value_type;
    using TIdx = typename TMatrix::index_type;

    SparseStream(TMatrix& A, TVector& v, TIdx stripSize, TIdx windowSize)
        : Stream<TVal, TIdx>(stream_direction::down), A_(A), v_(v),
          stripSize_(stripSize), windowSize_(windowSize) {}

    ~SparseStream() {
        for (TIdx s = 0; s < stream_config::processors; ++s) {
            operator delete(sparseData_[s]);
        }
    }

    void create() const override {
        for (TIdx s = 0; s < stream_config::processors; s++) {
            ebsp_create_down_stream_raw((const void*)(this->sparseData_[s]), s,
                                        0,  // total size FIXME
                                        0); // max chunk size (needed?) FIXME
        }
    }

    void prepareStream() {
        ZeeLogDebug << "SparseStream::prepareStream()" << endLog;

        ZeeAssert(A_.getRows() > windowSize_ && A_.getCols() > stripSize_);

        ZeeLogVar(A_.getRows());
        ZeeLogVar(A_.getCols());
        ZeeLogVar(A_.nonZeros());

        ZeeAssert(sizeof(TIdx) == sizeof(uint32_t));

        ZeeAssert(A_.getProcs() == stream_config::processors);

        // TODO: "windows" and "strips" should be constructed in some
        // partitioner, stored in matrix itself?
        TIdx strips = (A_.getCols() - 1) / stripSize_ + 1;
        TIdx windows = (A_.getRows() - 1) / windowSize_ + 1;

        std::array<std::vector<SparseStreamWindow<TVal, TIdx>>,
                   stream_config::processors> windowChunks;

        std::array<std::vector<std::set<TIdx>>, stream_config::processors>
            windowRowset;

        std::array<std::vector<std::set<TIdx>>, stream_config::processors>
            windowColset;

        for (TIdx s = 0; s < stream_config::processors; ++s) {
            windowChunks[s].resize(strips * windows);
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

                windowChunks[s][windowIdx].triplets.push_back(triplet);
                windowRowset[s][windowIdx].insert(triplet.row());
                windowColset[s][windowIdx].insert(triplet.col());
            }
            ++s;
        }

        std::array<std::vector<SparseStreamStripHeader<TVal, TIdx>>,
                   stream_config::processors> stripHeaders;
        std::array<std::vector<std::vector<TIdx>>, stream_config::processors>
            stripIndicesV;

        for (TIdx s = 0; s < stream_config::processors; ++s) {
            stripHeaders[s].resize(strips);
            stripIndicesV[s].resize(strips);
            for (TIdx strip = 0; strip < strips; ++strip) {
                stripHeaders[s][strip].numWindows = windows;
            }
        }

        auto& owners = v_.getOwners();
        ZeeLogVar(owners);

        // for each strip need local V
        for (TIdx strip = 0; strip < strips; ++strip) {
            // local V and values
            for (TIdx column = strip * stripSize_;
                 column < A_.getCols() && column < (strip + 1) * stripSize_;
                 ++column) {
                stripHeaders[owners[column]][strip].v.push_back(v_[column]);
                stripIndicesV[owners[column]][strip].push_back(column);
            }
        }

        // now we have a list of windows and strips
        // we now need:
        std::array<SparseStreamHeader<TIdx>, stream_config::processors>
            headers;
        for (TIdx s = 0; s < stream_config::processors; ++s) {
            for (TIdx windowIdx = 0; windowIdx < strips * windows;
                 ++windowIdx) {
                TIdx sizeV = windowColset[s][windowIdx].size();
                TIdx sizeU = windowRowset[s][windowIdx].size();
                TIdx sizeWindow = windowChunks[s][windowIdx].triplets.size();
                TIdx nonLocal =
                    sizeV - stripIndicesV[s][windowIdx / windows].size();

                windowChunks[s][windowIdx].nonLocalOwners.resize(nonLocal);
                windowChunks[s][windowIdx].nonLocalIndices.resize(nonLocal);
                windowChunks[s][windowIdx].sizeU = sizeU;
                windowSizeU_[s][windowIdx] = sizeU;
                upStreamSize_[s] += sizeU * sizeof(TIdx);

                if (sizeV > headers[s].maxSizeV)
                    headers[s].maxSizeV = sizeV;
                if (sizeU > headers[s].maxSizeU) {
                    headers[s].maxSizeU = sizeU;
                    upStreamChunkSize_[s] = sizeU;
                }
                if (sizeWindow > headers[s].maxWindowSize)
                    headers[s].maxWindowSize = sizeWindow;
                if (nonLocal > headers[s].maxNonLocal)
                    headers[s].maxNonLocal = nonLocal;
            }
        }

        for (TIdx s = 0; s < stream_config::processors; ++s) {
            localToGlobalU_[s].resize(strips * windows);
        }


        // localize strip indices
        std::array<std::map<TIdx, TIdx>, stream_config::processors>
            stripLocalIndices;

        for (TIdx s = 0; s < stream_config::processors; ++s) {
            for (TIdx strip = 0; strip < strips; ++strip) {
                // better to do this in one run.. here we localize the strip
                // indices using a local map
                for (TIdx i = 0; i < stripIndicesV[s][strip].size(); ++i) {
                    stripLocalIndices[s][stripIndicesV[s][strip][i]] = i;
                }
            }
        }

        for (TIdx s = 0; s < stream_config::processors; ++s) {
            for (TIdx strip = 0; strip < strips; ++strip) {
                for (TIdx window = 0; window < windows; ++window) {
                    std::map<TIdx, TIdx> windowLocalIndicesU;
                    auto windowIdx = strip * windows + window;

                    // NOTE: store mapping for u for every proc, otherwise we
                    // cannot gather
                    localToGlobalU_[s][windowIdx].resize(
                        windowRowset.size());
                    TIdx localIdx = 0;
                    for (auto row : windowRowset[s][windowIdx]) {
                        // localize U
                        windowLocalIndicesU[row] = localIdx;
                        localToGlobalU_[s][windowIdx][localIdx] =
                            row;
                        localIdx++;
                    }

                    // for each index we dont have, we need to set a local index
                    // and obtain the remote index.. hardddd?! no
                    // lets consider windowLocalIndices
                    std::map<TIdx, TIdx> windowLocalIndicesV =
                        stripLocalIndices[s]; // this is in a single strip
                    TIdx nonLocalIdx = stripLocalIndices[s].size();
                    for (auto col : windowColset[s][windowIdx]) {
                        if (windowLocalIndicesV.find(col) != windowLocalIndicesV.end()) {
                            windowLocalIndicesV[col] = nonLocalIdx++;
                        }
                        windowChunks[s][windowIdx].nonLocalOwners.push_back(owners[col]);
                        windowChunks[s][windowIdx].nonLocalIndices.push_back(stripLocalIndices[owners[col]][col]);
                    }

                    // localize window indices. Note that some indices are nonlocal
                    // here we need to find these and insert them in 'windowLocalIndices'
                    for (auto& triplet :
                         windowChunks[s][windowIdx].triplets) {
                        triplet.setCol(windowLocalIndicesV[triplet.col()]);
                        triplet.setRow(windowLocalIndicesU[triplet.row()]);
                    }
                }
            }
        }

        std::array<TIdx, stream_config::processors> streamSize;
        for (TIdx s = 0; s < stream_config::processors; ++s) {
            streamSize[s] = headers[s].sizeInBytes();
            for (TIdx strip = 0; strip < strips; ++strip) {
                streamSize[s] += stripHeaders[s][strip].sizeInBytes();
                for (TIdx window = 0; window < windows; ++window) {
                    TIdx windowIdx = strip * windows + window;
                    streamSize[s] += windowChunks[s][windowIdx].sizeInBytes();
                }
            }

            ZeeLogVar(streamSize[s]);
            sparseData_[s] = operator new(streamSize[s]);

            headers[s].write(&sparseData_[s]);
            for (TIdx strip = 0; strip < strips; ++strip) {
                stripHeaders[s][strip].write(&sparseData_[s]);
                for (TIdx window = 0; window < windows; ++window) {
                    TIdx windowIdx = strip * windows + window;
                    windowChunks[s][windowIdx].write(&sparseData_[s]);
                }
            }
        }

        ZeeLogDebug << "Finished constructing stream" << endLog;
    }

    TIdx upStreamSize(TIdx proc) const {
        return upStreamSize_[proc];
    }

    TIdx upStreamChunkSize(TIdx proc) const {
        return upStreamChunkSize_[proc];
    }

    const std::array<std::vector<std::vector<TIdx>>, stream_config::processors>&
    getLocalToGlobalU() const {
        return localToGlobalU_;
    }

    const std::array<std::vector<TIdx>, stream_config::processors>&
    getWindowSizeU() const {
        return windowSizeU_;
    }

  private:
    TMatrix& A_;
    TVector& v_;

    TIdx stripSize_;
    TIdx windowSize_;

    std::array<std::vector<std::vector<TIdx>>, stream_config::processors>
        localToGlobalU_;

    std::array<TIdx, stream_config::processors> upStreamSize_;
    std::array<TIdx, stream_config::processors> upStreamChunkSize_;
    std::array<std::vector<TIdx>, stream_config::processors> windowSizeU_;

    std::array<void*, stream_config::processors> sparseData_;
};

template <typename TVal, typename TIdx>
class SpMVUpStream
    : UpStream<TVal, TIdx> {
  public:
    SpMVUpStream() : chunkSizes_(), totalSizes_() {}

    void createUp() override {
        ZeeAssert(this->chunkSize_ != 0);
        ZeeAssert(this->totalSize_ != 0);

        TIdx totalSize = this->getTotalSize();
        TIdx chunkSize = this->getChunkSize();

        for (TIdx s = 0; s < stream_config::processors; s++) {
            this->rawData_[s] =
                (TVal*)ebsp_create_up_stream(s, totalSize, chunkSize);
        }
    }

    void setChunkSize(TIdx proc, TIdx size) {
        chunkSizes_[proc] = size;
    }

    void setTotalSize(TIdx proc, TIdx size) {
        totalSizes_[proc] = size;
    }

    void fill(DStreamingVector<TVal, TIdx>& u,
              const SparseStream<DStreamingSparseMatrix<TVal, TIdx>,
                                 DStreamingVector<TVal, TIdx>>& downStream) {
        auto& localToGlobalU = downStream.getLocalToGlobalU();

        for (TIdx s = 0; s < stream_config::processors; ++s) {
            TIdx offset = 0;
            TIdx windowIdx = 0;
            TIdx globalIdx = 0;
            TVal* data = this->rawData_[s];
            while (offset < totalSizes_[s]) {
                for (TIdx idx = 0; idx < downStream.getWindowSizeU()[s][windowIdx];
                     ++idx) {
                    u.at(localToGlobalU[s][windowIdx][idx]) = data[globalIdx++];
                }
                ++windowIdx;
            }
        }
    }

  private:
    // these are per processor
    std::array<TIdx, stream_config::processors> chunkSizes_ = 0;
    std::array<TIdx, stream_config::processors> totalSizes_ = 0;
};

} // namespace Zephany
