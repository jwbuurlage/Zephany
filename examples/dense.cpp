#include <string>
#include <cmath>

#include <zee.hpp>
#include <zephany.hpp>

using namespace Zee;
using namespace Zephany;

int main() {
    using TVal = float;
    using TIdx = unsigned int;

    ZeeLogInfo << "-- Starting dense example" << endLog;

    // DENSE
    int n = 256;

    DStreamingMatrix<TVal, TIdx> A(n);
    DStreamingMatrix<TVal, TIdx> B(n);

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            A.at(i, j) = (float)i;
            B.at(i, j) = (float)j;
        }

    // Until we find a better way, we need to set the stream orientation
    // explicitely. Maybe set this at the start and change 'at', but everything
    // is awefully slow anyway.. Maybe just iterate over blocks and fill with
    // lambda, mtx, ...
    B.getStream().setOrientation(stream_orientation::right_handed);

    DStreamingMatrix<TVal, TIdx> C(n); // block size gets set
    C = A * B;                            // moves
    ZeeLogVar(C.at(n - 1, n - 1));

    return 0;
}
