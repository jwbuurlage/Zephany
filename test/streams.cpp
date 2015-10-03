#include "catch.hpp"

#include <zephany.hpp>

#include <algorithm>

using namespace Zephany;

using TIdx = uint32_t;
using TVal = float;

TEST_CASE("simple stream construction and manipulation", "[streams]") {
    TIdx n = 16;
    TIdx k = 8;
    TIdx l = 2;
    TIdx N = 4;

    DStreamingMatrix<TVal, TIdx> matrix(l, n);
    auto& stream = matrix.getStream();

    TVal val = 0.0f;
    for (TIdx i = 0; i < n; ++i) {
        for (TIdx j = 0; j < n; ++j) {
            matrix.at(i, j) = val;
            val += 1.0f;
        }
    }

    std::array<TIdx, 3> procs = {0, 1, 4};

    std::array<std::array<float, 4>, 3> firstBlock = {
        0.0f,  1.0f,  16.0f, 17.0f, 2.0f,  3.0f,
        18.0f, 19.0f, 32.0f, 33.0f, 48.0f, 49.0f};

    std::array<std::array<float, 4>, 3> secondBlock = {
        8.0f,  9.0f,  24.0f, 25.0f, 10.0f, 11.0f,
        26.0f, 27.0f, 40.0f, 41.0f, 56.0f, 57.0f};

    std::array<std::array<float, 4>, 3> thirdBlock = {
        128.0f, 129.0f, 144.0f, 145.0f, 130.0f, 131.0f,
        146.0f, 147.0f, 160.0f, 161.0f, 176.0f, 177.0f};

    SECTION("the stream is LHS by default") {
        auto& data = stream.getData();

        for (unsigned int i = 0; i < 3; ++i) {
            CAPTURE(i);
            REQUIRE(std::equal(firstBlock[i].begin(), firstBlock[i].end(),
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
            CAPTURE(i);
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

void testMatrix(TIdx size) {
    TIdx blockSize = std::min((TIdx)32, size / (2 * stream_config::N));

    DStreamingMatrix<TVal, TIdx> A(blockSize, size);
    DStreamingMatrix<TVal, TIdx> B(blockSize, size);
    for (TIdx i = 0; i < size; ++i)
        for (TIdx j = 0; j < size; ++j) {
            A.at(i, j) = (float)i;
            B.at(i, j) = (float)j;
        }
    B.getStream().setOrientation(stream_orientation::right_handed);

    DStreamingMatrix<TVal, TIdx> C(blockSize, size);
    C = A * B;
    REQUIRE(C.at(1, 1) == size);
}

TEST_CASE("matrices of different sizes", "[streams]") {
    std::vector<TIdx> sizes = {16, 17, 32, 33, 64, 100, 128};
    for (auto size : sizes) {
        testMatrix(size);
    }
}

TEST_CASE("we can multiply two streamed matrices", "[streams]") {
    TIdx n = 256;

    DStreamingMatrix<TVal, TIdx> A(n);
    DStreamingMatrix<TVal, TIdx> B(n);

    for (TIdx i = 0; i < n; ++i)
        for (TIdx j = 0; j < n; ++j) {
            A.at(i, j) = (float)i;
            B.at(i, j) = (float)j;
        }

    A.getStream().setOrientation(stream_orientation::left_handed);
    B.getStream().setOrientation(stream_orientation::right_handed);

    DStreamingMatrix<TVal, TIdx> C(n);
    C = A * B;

    REQUIRE(C.at(n - 1, n - 1) == 16646400.0f);
}
