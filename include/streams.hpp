#pragma once

#include <zee.hpp>

extern "C" {
#include <host_bsp.h>
}

#include <vector>
#include <algorithm>

#ifndef ZEPHANY_MESH_SIZE
#define ZEPHANY_MESH_SIZE 4
#endif

namespace Zephany {

namespace stream_config {
static constexpr unsigned int N = ZEPHANY_MESH_SIZE;
static constexpr unsigned int processors = N * N;
}

enum class stream_direction { up, down };

template <typename T, typename TIdx = Zee::default_index_type>
class Stream {
  public:
    Stream(stream_direction direction) : direction_(direction) {}

    void setChunkSize(TIdx chunkSize) { chunkSize_ = chunkSize; }
    void setTotalSize(TIdx totalSize) { totalSize_ = totalSize; }

    TIdx getChunkSize() const { return chunkSize_; }
    TIdx getTotalSize() const { return totalSize_; }

    std::array<std::vector<T>, stream_config::processors>& getData() {
        return data_;
    }

    void setInitialized() { initialized_ = true; }

    virtual void create() const = 0;

  protected:
    // these are per processor
    TIdx chunkSize_ = 0;
    TIdx totalSize_ = 0;

    // we support upwards and downward streams
    stream_direction direction_ = stream_direction::down;

    std::array<std::vector<T>, stream_config::processors> data_;
    bool initialized_ = false;
};

template <typename T, typename TIdx = Zee::default_index_type>
class UpStream : public Stream<T, TIdx> {
  public:
    UpStream() : Stream<T, TIdx>(stream_direction::up) {}

    void create() const override {}

    void createUp() {
        ZeeAssert(this->chunkSize_ != 0);
        ZeeAssert(this->totalSize_ != 0);

        TIdx totalSize = this->getTotalSize();
        TIdx chunkSize = this->getChunkSize();

        for (TIdx s = 0; s < stream_config::processors; s++) {
            this->rawData_[s] = (T*)ebsp_create_up_stream(s, totalSize, chunkSize);
        }
    }

    const std::array<T*, stream_config::processors>& getRawData() const {
        return rawData_;
    }

  private:
    std::array<T*, stream_config::processors> rawData_;
};

/* Stream definition for an n x n dense matrix.
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

template <typename T, typename TIdx = Zee::default_index_type>
class MatrixBlockStream : public Stream<T, TIdx> {
  public:
    MatrixBlockStream(stream_direction direction)
        : Stream<T, TIdx>(direction) {}

    /* Switch stream arrangement (for LHS/RHS of matrix operations) */
    void setOrientation(stream_orientation orientation) {
        if (orientation_ == orientation)
            return;

        transposeStream_();

        orientation_ = orientation;
    }

    stream_orientation getOrientation() const { return orientation_; }

    void setInner(TIdx count, TIdx size) {
        innerBlocks_ = count;
        innerBlockSize_ = size;
    }

    void setOuter(TIdx count, TIdx size) {
        outerBlocks_ = count;
        outerBlockSize_ = size;
    }

    TIdx getInnerBlockSize() const { return innerBlockSize_; }
    TIdx getOuterBlocks() const { return outerBlocks_; }
    TIdx getOuterBlockSize() const { return outerBlockSize_; }

    void setMatrixSize(TIdx matrixSize) { matrixSize_ = matrixSize; }

    void reshape() {
        for (TIdx s = 0; s < stream_config::processors; ++s) {
            this->data_[s].resize(outerBlocks_ * outerBlocks_ *
                                  innerBlockSize_ * innerBlockSize_);
        }
    }

    // Note: This is really show, and should not be used to loop over matrix
    // we always optimize for size on Epiphany.
    T& element(TIdx i, TIdx j) {
        // see which global block:
        TIdx outerBlockI = i / outerBlockSize_;
        TIdx outerBlockJ = j / outerBlockSize_;

        i -= outerBlockI * outerBlockSize_;
        j -= outerBlockJ * outerBlockSize_;

        // Get inner block
        TIdx innerBlockI = i / innerBlockSize_;
        TIdx innerBlockJ = j / innerBlockSize_;

        i -= innerBlockI * innerBlockSize_;
        j -= innerBlockJ * innerBlockSize_;

        return this
            ->data_[innerBlockI * innerBlocks_ +
                    innerBlockJ][(outerBlockI * outerBlocks_ + outerBlockJ) *
                                     innerBlockSize_ * innerBlockSize_ +
                                 i * innerBlockSize_ + j];
    }

    const T& element(TIdx i, TIdx j) const {
        // see which global block:
        TIdx outerBlockI = i / outerBlockSize_;
        TIdx outerBlockJ = j / outerBlockSize_;

        i -= outerBlockI * outerBlockSize_;
        j -= outerBlockJ * outerBlockSize_;

        // Get inner block
        TIdx innerBlockI = i / innerBlockSize_;
        TIdx innerBlockJ = j / innerBlockSize_;

        i -= innerBlockI * innerBlockSize_;
        j -= innerBlockJ * innerBlockSize_;

        return this
            ->data_[innerBlockI * innerBlocks_ +
                    innerBlockJ][(outerBlockI * outerBlocks_ + outerBlockJ) *
                                     innerBlockSize_ * innerBlockSize_ +
                                 i * innerBlockSize_ + j];
    }

    void computeChunkSize() {
        ZeeAssert(innerBlocks_ != 0);
        ZeeAssert(innerBlockSize_ != 0);
        ZeeAssert(outerBlocks_ != 0);
        ZeeAssert(matrixSize_ != 0);

        this->chunkSize_ = innerBlockSize_ * innerBlockSize_ * sizeof(T);
        this->totalSize_ = outerBlocks_ * outerBlocks_ * this->chunkSize_;
    }

    void create() const override {
        ZeeAssert(this->chunkSize_ != 0);
        ZeeAssert(this->totalSize_ != 0);

        TIdx totalSize = this->getTotalSize();
        TIdx chunkSize = this->getChunkSize();
        for (TIdx s = 0; s < stream_config::processors; s++) {
            ebsp_create_down_stream((void*)(this->data_[s].data()), s, totalSize,
                                    chunkSize);
        }
    }

  private:
    void transposeStream_() {
        // row major blocks to column major
        TIdx chunkElements = this->innerBlockSize_ * this->innerBlockSize_;
        for (TIdx s = 0; s < stream_config::N * stream_config::N; ++s) {
            for (TIdx chunkI = 0; chunkI < outerBlocks_; ++chunkI)
                for (TIdx chunkJ = chunkI + 1; chunkJ < outerBlocks_; ++chunkJ) {
                    TIdx chunkOriginal = chunkI * outerBlocks_ + chunkJ;
                    TIdx chunkTarget = chunkJ * outerBlocks_ + chunkI;
                    TIdx offset = chunkOriginal * chunkElements;
                    TIdx targetOffset = chunkTarget * chunkElements;
                    std::swap_ranges(this->data_[s].begin() + offset,
                                     this->data_[s].begin() + offset +
                                         chunkElements,
                                     this->data_[s].begin() + targetOffset);
                }
        }
    }

    stream_orientation orientation_ = stream_orientation::left_handed;
    TIdx innerBlocks_ = 0;
    TIdx innerBlockSize_ = 0;
    TIdx outerBlocks_ = 0;
    TIdx outerBlockSize_ = 0;
    TIdx matrixSize_ = 0;
};

} // namespace Zephany
