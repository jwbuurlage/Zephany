#pragma once

#include <zee.hpp>

extern "C" {
#include <host_bsp.h>
}

#include <vector>
#include <algorithm>

namespace Zephany {

namespace stream_config {
constexpr int N = 4;
constexpr int processors = N * N;
}

enum class stream_direction { up, down };

template <typename T>
class Stream {
  public:
    Stream(stream_direction direction) : direction_(direction) {}

    void setChunkSize(int chunkSize) { chunkSize_ = chunkSize; }
    void setTotalSize(int totalSize) { totalSize_ = totalSize; }
    void setInitialized() { initialized_ = true; }

    virtual void create() = 0;

  protected:
    // these are per processor
    int chunkSize_ = 0;
    int totalSize_ = 0;

    // we support upwards and downward streams
    stream_direction direction_ = false;

    std::array<std::vector<T>, stream_config::processors> data_;
    bool initialized_ = false;
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
    void setRepresentation(stream_orientation orientation) {
        if (orientation_ == orientation)
            return;

        transposeStream_();

        orientation_ = orientation;
    }

    void create() override {
        // bsp this and that for each processor and such
        for (int s = 0; s < stream_config::N * stream_config::N; s++) {
            //ebsp_create_down_stream(&(this->data_[s]), s, this->totalSize_,
            //                        this->chunkSize_);
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

        ZeeLogVar(this->data_[0]);
    }

    void setInner(int count, int size) {
        innerBlocks_ = count;
        innerBlockSize_ = size;
    }

    void setOuter(int count, int size) {
        outerBlocks_ = count;
        outerBlockSize_ = size;
    }

    void setMatrixSize(int matrixSize) { matrixSize_ = matrixSize; }

    void computeChunkSize() {
        ZeeAssert(innerBlocks_ != 0);
        ZeeAssert(innerBlockSize_ != 0);
        ZeeAssert(outerBlocks_ != 0);
        ZeeAssert(matrixSize_ != 0);

        this->chunkSize_ = innerBlockSize_ * innerBlockSize_ * sizeof(T);
        this->totalSize_ = outerBlocks_ * outerBlocks_ * this->chunkSize_;

        ZeeLogVar(this->chunkSize_);
        ZeeLogVar(this->totalSize_);
    }

  private:
    void transposeStream_() {
        int sizePerChunkRow = this->chunkSize_ * outerBlocks_;
        // row major blocks to column major
        for (int s = 0; s < stream_config::N * stream_config::N; ++s) {
            for (int chunkI = 1; chunkI < outerBlocks_; ++chunkI)
            for (int chunkJ = 0; chunkJ < chunkI; ++chunkJ) {
                int offset = chunkI * sizePerChunkRow + chunkJ * this->chunkSize_;
                int targetOffset =
                    chunkJ * sizePerChunkRow + chunkI * this->chunkSize_;
                std::swap_ranges(this->data[s].begin() + offset,
                                 this->data[s].begin() + offset +
                                     this->chunkSize_,
                                 this->data[s].begin() + targetOffset);
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
