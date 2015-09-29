#pragma once

#include <zee.hpp>

#include <vector>

namespace zephany {

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

    virtual void feed(const std::vector<T>& data) = 0;
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

        // fix if necessary
        // isRowMajor_ = rowMajor;
    }

    void create() override {
        // bsp this and that for each processor and such
        //            for (int s = 0; s < N * N; s++) {
        //                ebsp_create_down_stream(DATA,
        //                        s,
        //                        TOTAL_STREAM_SIZE,
        //                        CHUNK_SIZE);
        //            }
    }

    void feed(const std::vector<T>& data) override {
        ZeeAssert(this->chunkSize_ != 0);
        ZeeAssert(this->totalSize_ != 0);

        for (int s = 0; s < stream_config::processors; ++s) {
            this->data_[s].reserve(this->totalSize_);
        }

        for (auto& elem : data) {
            // here we make the stream
            ZeeLogVar(elem);
        }

    //    for (int block = 0; block < block_count * block_count; ++block) {
    //        int blockI = block / block_count;
    //        int blockJ = block % block_count;
    //        int baseColumn = blockJ * BLOCK_SIZE;
    //        int baseRow = blockI * BLOCK_SIZE;
    //        for (int proc = 0; proc < N * N; ++proc) {
    //            int s = proc / N;
    //            int t = proc % N;
    //            int coreBlockColumn = baseColumn + t * CORE_BLOCK_SIZE;
    //            int coreBlockRow = baseRow + s * CORE_BLOCK_SIZE;
    //            for (int i = 0; i < CORE_BLOCK_SIZE; ++i) {
    //                for (int j = 0; j < CORE_BLOCK_SIZE; ++j) {
    //                    C[(coreBlockRow + i) * matrix_size + coreBlockColumn + j] =
    //                        up_streams[proc][cur_index[proc]++];
    //                }
    //            }
    //        }
    //    }
    }

  private:
    stream_orientation orientation_ = stream_orientation::left_handed;
    int innerBlocks_ = 0;
    int innerBlockSize_ = 0;
    int outerBlocks_ = 0;
    int matrixSize_ = 0;
};

} // namespace Zephany
