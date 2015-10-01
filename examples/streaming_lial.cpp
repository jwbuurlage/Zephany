#include <string>
#include <cmath>

#include <zee.hpp>
#include <zephany.hpp>

using namespace Zee;
using namespace Zephany;

int main() {
    using TVal = float;
    using TIdx = int;

    ZeeLogInfo << "-- Starting Epiphany example" << endLog;

    // DENSE
    int n = 64;

    DStreamingMatrix<TVal, TIdx> A(n, n);
    DStreamingMatrix<TVal, TIdx> B(n, n);

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            A.at(i, j) = (float)i;
            B.at(i, j) = (float)j;
        }
    A.updateStream();
    B.updateStream();
    A.getStream().setOrientation(stream_orientation::left_handed);
    B.getStream().setOrientation(stream_orientation::right_handed);

    DStreamingMatrix<TVal, TIdx> C(n, n);
    C = A * B;
    ZeeLogVar(C.at(0, 0));

    // SPARSE
    // We initialize the matrix with cyclic distribution
    /* std::string matrix = "steam3";
    auto S = DStreamingSparseMatrix<TVal,
    TIdx>("/home/jw/zephany/data/matrices/" + matrix + ".mtx", 4);
    DStreamingVector<TVal, TIdx> x(S.getCols(), 1.0);
    DStreamingVector<TVal, TIdx> y(S.getRows(), 1.0);
    y = S * x;
    y = S * x; */

    return 0;
}
