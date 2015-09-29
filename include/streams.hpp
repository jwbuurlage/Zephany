#include <zee.hpp>

#include <vector>

namespace stream_config {
constexpr int N = 4;
constexpr int processors = N * N;
}

namespace Zephany {

class enum stream_direction {
    up,
    down
};

template <typename T>
    class Stream
    {
        public:
            Stream(stream_direction direction)
                : direction_(direction)
            { }

            void setChunkSize(int chunkSize) {
                chunkSize_ = chunkSize;
            }
            void setTotalSize(int totalSize) {
                totalSize_ = totalSize;
            }

            void setInitialized() {
                initialized_ = true;
            }

            virtual void feed(const vector<T>& data) = 0;

            virtual void create() = 0;

        protected:
            int chunkSize_ = 0;
            int totalSize_ = 0;
            bool isDownStream_ = false;
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

class enum representation
{
    row_major,
    column_major
};

template <typename T>
    class MatrixBlockStream : public Stream<T>
{
    public:
        MatrixBlockStream(int chunkSize,
                stream_direction direction,
                int processors)
            : Stream<T>(chunkSize, direction, processors)
        {
        }

        /* Switch stream arrangement (for LHS/RHS of matrix operations) */
        void setRepresentation(representation rep) {
            if (rep == representation_)
                return;

            // fix if necessary
            isRowMajor_ = rowMajor;
        }

        void create() override
        {
            // bsp this and that for each processor and such
            for (int s = 0; s < N * N; s++) {
                ebsp_create_down_stream(DATA,
                        s,
                        TOTAL_STREAM_SIZE,
                        CHUNK_SIZE);
            }
        }

    private:
        // LHS orientation is row major,
        // RHS orientation is column major
        representation representation_ = representation::row_major;
        int innerBlocks_ = 0;
        int outerBlocks_ = 0;
        int matrixSize_ = 0;
};

} // namespace Zephany
