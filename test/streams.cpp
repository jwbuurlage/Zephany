#include "catch.hpp"

#include <zephany.hpp>

using TIdx = uint32_t;
using TVal = float;

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

TEST_CASE("stream construction", "[streams]") {
    // FIXME test transpose (lhs/rhs)
    // FIXME test stream construction
}
