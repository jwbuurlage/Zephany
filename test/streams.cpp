#include "catch.hpp"

#include <zephany.hpp>

#include <algorithm>

using namespace Zephany;

using TIdx = uint32_t;
using TVal = float;

TEST_CASE("simple stream construction and manipulation", "[streams]") {
    REQUIRE(0 == 0);

    int n = 16;
    int k = 8;
    int l = 2;
    int N = 4;

    MatrixBlockStream<TVal> stream(stream_direction::up);
    stream.setInner(N, l);
    stream.setOuter(n / k, k);
    stream.setMatrixSize(n);
    stream.computeChunkSize();

    std::vector<std::vector<float>> matrix(n, std::vector<float>(n, 0.0));
    TVal val = 0.0f;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            matrix[i][j] = val;
            val += 1.0f;
        }
    }

    stream.feedElements(matrix);

    std::array<TIdx, 3> procs = { 0, 1, 4 };

    std::array<std::array<float, 4>, 3> firstBlock = {
        0.0f, 1.0f, 16.0f, 17.0f,
        2.0f, 3.0f, 18.0f, 19.0f,
        32.0f, 33.0f, 48.0f, 49.0f
    };

    std::array<std::array<float, 4>, 3> secondBlock = {
        8.0f, 9.0f, 24.0f, 25.0f,
        10.0f, 11.0f, 26.0f, 27.0f,
        40.0f, 41.0f, 56.0f, 57.0f
    };

    std::array<std::array<float, 4>, 3> thirdBlock = {
        128.0f, 129.0f, 144.0f, 145.0f,
        130.0f, 131.0f, 146.0f, 147.0f,
        160.0f, 161.0f, 176.0f, 177.0f
    };

    SECTION("the stream is LHS by default") {
        auto& data = stream.getData();

        for (unsigned int i = 0; i < 3; ++i) {
            CHECK(std::equal(firstBlock[i].begin(), firstBlock[i].end(),
                               data[procs[i]].begin()));
        }

        for (unsigned int i = 0; i < 3; ++i) {
            CAPTURE(i);
            CHECK(std::equal(secondBlock[i].begin(), secondBlock[i].end(),
                               data[procs[i]].begin() + l * l));
        }

        for (unsigned int i = 0; i < 3; ++i) {
            CAPTURE(i);
            CHECK(std::equal(thirdBlock[i].begin(), thirdBlock[i].end(),
                               data[procs[i]].begin() + 2 * l * l));
        }
    }

    SECTION("we can transpose the stream") {
        stream.setOrientation(stream_orientation::right_handed);
        auto& data = stream.getData();

        for (unsigned int i = 0; i < 3; ++i) {
            CHECK(std::equal(firstBlock[i].begin(), firstBlock[i].end(),
                               data[procs[i]].begin()));
        }

        for (unsigned int i = 0; i < 3; ++i) {
            CAPTURE(i);
            CHECK(std::equal(thirdBlock[i].begin(), thirdBlock[i].end(),
                               data[procs[i]].begin() + l * l));
        }

        for (unsigned int i = 0; i < 3; ++i) {
            CAPTURE(i);
            CHECK(std::equal(secondBlock[i].begin(), secondBlock[i].end(),
                               data[procs[i]].begin() + 2 * l * l));
        }
    }
}

TEST_CASE("stream construction and manipulation using matrices", "[streams]") {
    const int n = 128;

    DStreamingMatrix<TVal, TIdx> A(n, n);
    DStreamingMatrix<TVal, TIdx> B(n, n);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            A.at(i, j) = (float)i;
            B.at(i, j) = (float)j;
        }
    }

    A.updateStream();
    B.updateStream();
    A.getStream().setOrientation(stream_orientation::left_handed);
    B.getStream().setOrientation(stream_orientation::right_handed);

    auto& lhsStream = A.getStream();
    auto& rhsStream = B.getStream();

    int k = lhsStream.getInnerBlockSize();
    int l = lhsStream.getOuterBlockSize();
    int m = lhsStream.getOuterBlocks();

    SECTION("we can construct the lhs stream using matrix") {
        auto& dataLHS = lhsStream.getData();
        std::vector<float> head(k, 0.0);
        for (int s = 0; s < stream_config::processors; ++s) {
            // first block A[0] should be:
            // 0 1 2 3 .. l
            // 0 1 2 3 .. l
            // . . . . .. .
            std::iota(head.begin(), head.end(), (float)((s / stream_config::N) * k));
            CHECK(std::equal(dataLHS[s].begin(), dataLHS[s].begin() + k,
                             head.begin()));

            // second block should be equal to the first
            // 0 1 2 3 .. k
            // 0 1 2 3 .. k
            // . . . . .. .
            CHECK(std::equal(dataLHS[s].begin() + k * k,
                             dataLHS[s].begin() + 2 * k * k,
                             dataLHS[s].begin()));
        }
    }

    SECTION("we can construct the rhs stream using matrix") {
        auto& dataRHS = rhsStream.getData();
        for (int s = 0; s < stream_config::processors; ++s) {
            //first block
            // 0 0 0 0 .. 0
            // 1 1 1 1 .. 1
            // . . . . .. .
            // l l l l .. l


            CAPTURE(s);
            CAPTURE(dataRHS[s][0]);
            CAPTURE(dataRHS[s][1]);

            // first row of first block should be zero
            CHECK(std::all_of(
                dataRHS[s].begin(), dataRHS[s].begin() + k, [s, k](const float& a) {
                    return a == (float)((s / stream_config::N) * k);
                }));

            // second block
//            CHECK(dataRHS[s][k] == (float)(s % stream_config::N));
//
//            // (M + 1)th block
//            CHECK(dataRHS[s][m * k] ==
//                  (float)(stream_config::N * k +
//                          (s % stream_config::N)));
        }
    }
}

TEST_CASE("we can multiply two streamed matrices", "[streams]") {
    // dense A * B = C
    //    DStreamingMatrix<TVal, TIdx> C(n, n);
    // C = A * B;


}
