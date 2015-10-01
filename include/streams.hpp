#pragma once

#include <zee.hpp>

extern "C" {
#include <host_bsp.h>
}

#include <vector>
#include <algorithm>

namespace Zephany {

namespace stream_config {
static constexpr int N = 4;
static constexpr int processors = N * N;
}

enum class stream_direction { up, down };

template <typename T>
class Stream {
  public:
    Stream(stream_direction direction) : direction_(direction) {}

    void setChunkSize(int chunkSize) { chunkSize_ = chunkSize; }
    void setTotalSize(int totalSize) { totalSize_ = totalSize; }

    int getChunkSize() const { return chunkSize_; }
    int getTotalSize() const { return totalSize_; }

    std::array<std::vector<T>, stream_config::processors>& getData() {
        return data_;
    }

    void setInitialized() { initialized_ = true; }

    virtual void create() const = 0;

  protected:
    // these are per processor
    int chunkSize_ = 0;
    int totalSize_ = 0;

    // we support upwards and downward streams
    stream_direction direction_ = stream_direction::down;

    std::array<std::vector<T>, stream_config::processors> data_;
    bool initialized_ = false;
};

template <typename T>
class UpStream : public Stream<T> {
  public:
    UpStream() : Stream<T>(stream_direction::up) {}

    void create() const override {}

    void createUp() {
        int totalSize = this->getTotalSize();
        int chunkSize = this->getChunkSize();
        for (int s = 0; s < stream_config::N * stream_config::N; s++) {
            this->rawData_[s] = (T*)ebsp_create_up_stream(s, totalSize, chunkSize);
        }
    }

    const std::array<T*, stream_config::processors>& getRawData() const {
        return rawData_;
    }

  private:
    std::array<T*, stream_config::processors> rawData_;
};

/* Stream definition for an n x n (TODO: rectangular) dense matrix.
 * for a processor mesh of size N x N. The matrix is stored
 * in M x M 'outer blocks', and each outer block is split into
 * N x N smaller inner blocks.
 *
 * isRowMajor_ defines the orientation: In an operation C = A * B
 * we want to store:
 * A: A_11 A_12 ... A_1M (repeat M) A_21 ... A_2M (repeat M) ... A_MM (repeat M)
 * B: B_11 B_21 ... B_M1 B_12 ... B_M2 ... B_MM (repeat M)
 * We call orientation of A (left-hand side) row major, and B column major
 * and we need to be able to switch between these oreitnations.
 */

// LHS orientation is row major,
// RHS orientation is column major
enum class stream_orientation { left_handed, right_handed };

template <typename T>
class MatrixBlockStream : public Stream<T> {
  public:
    MatrixBlockStream(stream_direction direction) : Stream<T>(direction) {}

    /* Switch stream arrangement (for LHS/RHS of matrix operations) */
    void setOrientation(stream_orientation orientation) {
        if (orientation_ == orientation)
            return;

        transposeStream_();

        orientation_ = orientation;
    }

    void create() const override {
        int totalSize = this->getTotalSize();
        int chunkSize = this->getChunkSize();
        for (int s = 0; s < stream_config::N * stream_config::N; s++) {
            ebsp_create_down_stream((void*)this->data_[s].data(), s, totalSize,
                                    chunkSize);
        }
    }

    void feedElements(const std::vector<std::vector<T>>& data) {
        ZeeAssert(this->chunkSize_ != 0);
        ZeeAssert(this->totalSize_ != 0);

        for (int s = 0; s < stream_config::processors; ++s) {
            this->data_[s].reserve(this->totalSize_);
        }

        // This is always laid out left-handed (row major)
        for (int blockI = 0; blockI < outerBlocks_; ++blockI)
        for (int blockJ = 0; blockJ < outerBlocks_; ++blockJ) {
            int blockOffsetI = blockI * outerBlockSize_;
            int blockOffsetJ = blockJ * outerBlockSize_;
            for (int s = 0; s < stream_config::N; ++s)
            for (int t = 0; t < stream_config::N; ++t) {
                int procOffsetI = s * innerBlockSize_;
                int procOffsetJ = t * innerBlockSize_;
                for (int i = 0; i < innerBlockSize_; ++i)
                for (int j = 0; j < innerBlockSize_; ++j) {
                    this->data_[s * stream_config::N + t].push_back(
                        data[blockOffsetI + procOffsetI + i][blockOffsetJ +
                                                             procOffsetJ + j]);
                }
            }
        }
    }

    void setInner(int count, int size) {
        innerBlocks_ = count;
        innerBlockSize_ = size;
    }

    void setOuter(int count, int size) {
        outerBlocks_ = count;
        outerBlockSize_ = size;
    }

    int getInnerBlockSize() const { return innerBlockSize_; }
    int getOuterBlocks() const { return outerBlocks_; }
    int getOuterBlockSize() const { return outerBlockSize_; }

    void setMatrixSize(int matrixSize) { matrixSize_ = matrixSize; }

    void computeChunkSize() {
        ZeeAssert(innerBlocks_ != 0);
        ZeeAssert(innerBlockSize_ != 0);
        ZeeAssert(outerBlocks_ != 0);
        ZeeAssert(matrixSize_ != 0);

        this->chunkSize_ = innerBlockSize_ * innerBlockSize_ * sizeof(T);
        this->totalSize_ = outerBlocks_ * outerBlocks_ * this->chunkSize_;
    }

  private:
    void transposeStream_() {
        // row major blocks to column major
        int chunkElements = this->innerBlockSize_ * this->innerBlockSize_;
        for (int s = 0; s < stream_config::N * stream_config::N; ++s) {
            for (int chunkI = 0; chunkI < outerBlocks_; ++chunkI)
                for (int chunkJ = chunkI + 1; chunkJ < outerBlocks_; ++chunkJ) {
                    int chunkOriginal = chunkI * outerBlocks_ + chunkJ;
                    int chunkTarget = chunkJ * outerBlocks_ + chunkI;
                    int offset = chunkOriginal * chunkElements;
                    int targetOffset = chunkTarget * chunkElements;
                    std::swap_ranges(this->data_[s].begin() + offset,
                                     this->data_[s].begin() + offset +
                                         chunkElements,
                                     this->data_[s].begin() + targetOffset);
                }
        }
    }

    stream_orientation orientation_ = stream_orientation::left_handed;
    int innerBlocks_ = 0;
    int innerBlockSize_ = 0;
    int outerBlocks_ = 0;
    int outerBlockSize_ = 0;
    int matrixSize_ = 0;
};

} // namespace Zephany
