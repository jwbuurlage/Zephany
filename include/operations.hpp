extern "C" {
#include <host_bsp.h>
}

namespace Zephany {

using namespace Zee;

/*** OPERATIONS ***/
template <typename TVal, typename TIdx>
DStreamingVector<TVal, TIdx> perform_operation(
        BinaryOperation<operation::type::product,
        DStreamingSparseMatrix<TVal, TIdx>,
        DStreamingVector<TVal, TIdx>> op)
{
    const auto& A = op.getLHS();
    const auto& x = op.getRHS();
    DStreamingVector<TVal, TIdx> y(A.getRows(), 1.0);

    ZeeLogInfo << "SpMV on Epiphany" << endLog;
    ZeeLogVar(A.nonZeros());
    ZeeLogVar(x.size());

    // Initialize the BSP system
    bsp_init("kernels/k_hello_world.srec", 0, 0);

    // Initialize the Epiphany system and load the binary
    bsp_begin(bsp_nprocs());

    // Run the program on the Epiphany cores
    ebsp_spmd();

    // Finalize
    bsp_end();

    return y;
}

template <typename TVal, typename TIdx>
DStreamingMatrix<TVal, TIdx> perform_operation(
        BinaryOperation<operation::type::product,
        DStreamingMatrix<TVal, TIdx>,
        DStreamingMatrix<TVal, TIdx>> op)
{
    ZeeLogDebug << "Starting dense-dense with cannon" << endLog;
    auto& A = op.getLHS();
    auto& B = op.getRHS();

    // FIXME: assuming 2^n size
    ZeeAssert((A.getRows() & (A.getRows() - 1)) == 0); // check if power of two
    ZeeAssert(A.getRows() == A.getCols()); // check if A is square
    ZeeAssert(A.getRows() == B.getRows()); // check if same size
    ZeeAssert(A.getCols() == B.getCols()); // check if B is square

    // FIXME if long enough

    // Initialize the BSP system
    bsp_init("kernels/k_cannon.srec", 0, 0);

    // FIXME: config?
    bsp_begin(16);
    int matrixSize = A.size();
    int blockSize = 4;
    int N = 4;
    int M = matrixSize / (N * blockSize);

    /* int N = 4;
     * for (int s = 0; s < N * N; s++) {
     *     ebsp_create_down_stream(stream_A[s], s, matrix_bytes / (N * N), CORE_BLOCK_BYTES);
     *     ebsp_create_down_stream(stream_B[s], s, matrix_bytes / (N * N), CORE_BLOCK_BYTES);
     *     ebsp_send_down(s, &tag, &M, sizeof(int));
     *     up_streams[s] = ebsp_create_up_stream(s,              // core id
     *             block_count * block_count * CORE_BLOCK_BYTES, // total size
     *             CORE_BLOCK_BYTES);                            // stream size
     * } */

    // open the streams
    ebsp_spmd();

    bsp_end();

    // need to check if both matrices have streams ready
    // or already in extmem -- etc.
    return DStreamingMatrix<TVal, TIdx>(1, 1);
}

} // namespace Zephany
