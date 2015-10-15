#include <string>
#include <cmath>

#include <zee.hpp>
#include <zephany.hpp>

using namespace Zephany;

int main() {
    using TVal = float;
    using TIdx = unsigned int;

    ZeeLogInfo << "-- Starting SpMV example" << endLog;

    // SPARSE
    // We initialize the matrix with cyclic distribution
    std::string matrix = "steam3";

    auto S = DStreamingSparseMatrix<TVal, TIdx>(
        "/home/jw/zephany/data/matrices/" + matrix + ".mtx", 4);
    DStreamingVector<TVal, TIdx> x(S.getCols(), 1.0);
    DStreamingVector<TVal, TIdx> y(S.getRows(), 1.0);

    ZeeLogVar(S.getCols());
    ZeeLogVar(S.getRows());

    Zee::MGPartitioner<decltype(S)> matrixPartitioner;
    matrixPartitioner.partition(S);

    GreedyVectorPartitioner<decltype(S), decltype(x)> vectorPartitioner(S, x, y);
    vectorPartitioner.partition();

    // create a strip / window "view"
    SparseStream<decltype(S), decltype(x)> sparseStream(S, x, 50, 50);

    sparseStream.prepareStream();
//
//    y = S * x;

    return 0;
}
