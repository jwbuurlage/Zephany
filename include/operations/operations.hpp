extern "C" {
#include <host_bsp.h>
}

#include "streams/streams.hpp"

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
    const auto& v = op.getRHS();
    DStreamingVector<TVal, TIdx> u(A.getRows(), 1.0);

    ZeeLogInfo << "SpMV on Epiphany" << endLog;
    ZeeLogVar(A.nonZeros());
    ZeeLogVar(v.size());

    // Initialize the BSP system
    bsp_init("kernels/k_spmv.srec", 0, 0);

    // Initialize the Epiphany system and load the binary
    bsp_begin(bsp_nprocs());

    const auto& stream = A.getStream();
    SpMVUpStream<TVal, TIdx> upStream;
    for (TIdx s = 0; s < stream_config::processors; ++s) {
        upStream.setChunkSize(s, stream.upStreamChunkSize(s));
        upStream.setTotalSize(s, stream.upStreamSize(s));
    }

    stream.create();
    upStream.createUp();

    // Run the program on the Epiphany cores
    ebsp_spmd();

    // Gather U
    upStream.fill(u, stream);

    // Finalize
    bsp_end();

    return u;
}

template <typename TVal, typename TIdx>
DStreamingMatrix<TVal, TIdx> perform_operation(
        BinaryOperation<operation::type::product,
        DStreamingMatrix<TVal, TIdx>,
        DStreamingMatrix<TVal, TIdx>> op)
{
    auto& A = op.getLHS();
    auto& B = op.getRHS();

    ZeeAssert(A.getCols() == B.getRows());

    // put result in new matrix C
    DStreamingMatrix<TVal, TIdx> C(A.getStream().getInnerBlockSize(),
                                   A.getRows());

    // Initialize the BSP system
    bsp_init("kernels/k_cannon.srec", 0, 0);

    bsp_begin(stream_config::processors);

    const auto& lhsStream = A.getStream();
    const auto& rhsStream = B.getStream();
    ZeeAssert(lhsStream.getOrientation() == stream_orientation::left_handed);
    ZeeAssert(rhsStream.getOrientation() == stream_orientation::right_handed);

    TIdx innerBlockSize = lhsStream.getInnerBlockSize();
    TIdx outerBlocks = lhsStream.getOuterBlocks();
    TIdx N = stream_config::N;

    UpStream<TVal> upStream;
    upStream.setChunkSize(innerBlockSize * innerBlockSize * sizeof(float));
    upStream.setTotalSize(outerBlocks * outerBlocks * innerBlockSize *
                          innerBlockSize * sizeof(float));

    lhsStream.create();
    rhsStream.create();
    upStream.createUp();

    // send Cannon parameters down to the kernel
    int tagsize = sizeof(int);
    ebsp_set_tagsize(&tagsize);

    for (TIdx s = 0; s < stream_config::processors; ++s) {
        int tag = 0;
        ebsp_send_down(s, &tag, &innerBlockSize, sizeof(int));
        tag = 1;
        ebsp_send_down(s, &tag, &outerBlocks, sizeof(int));
        tag = 2;
        ebsp_send_down(s, &tag, &N, sizeof(int));
    }

    ebsp_spmd();

    C.fillWithUpStream(upStream);

    bsp_end();

    return C;
}

} // namespace zephany
