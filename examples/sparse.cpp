#include <string>
#include <cmath>

#include <zee.hpp>
#include <zephany.hpp>

using namespace Zephany;

int main() {
    using TVal = float;
    using TIdx = unsigned int;
    using TVector = DStreamingVector<TVal, TIdx>;
    using TMatrix = DStreamingSparseMatrix<TVal, TIdx>;
    using TVectorPartitioner = GreedyVectorPartitioner<TMatrix, TVector>;

    ZeeLogInfo << "-- Starting SpMV example" << endLog;

    // SPARSE
    // We initialize the matrix with cyclic distribution
    std::string matrix = "steam3";

    // TODO: actually want to make random matrix, distribute by columns at first
    auto S = TMatrix("/home/jw/zephany/data/matrices/" + matrix + ".mtx",
                     stream_config::processors);
    TVector x(S.getCols(), 1.0);
    TVector y(S.getRows(), 1.0);

    //Zee::MGPartitioner<decltype(S)> matrixPartitioner;
    //matrixPartitioner.partition(S);

    TVectorPartitioner vectorPartitioner(S, x, y);
    vectorPartitioner.partition();

    // create a strip / window "view"
    SparseStream<decltype(S), decltype(x)> sparseStream(S, x, 50, 50);
    sparseStream.prepareStream();


    // TODO: create up stream

    // TODO: attach streams to S?

    // y = S * x;

    return 0;
}
