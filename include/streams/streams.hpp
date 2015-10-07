#pragma once

#include <zee.hpp>

extern "C" {
#include <host_bsp.h>
}

#include <vector>
#include <algorithm>

#ifndef ZEPHANY_MESH_SIZE
#define ZEPHANY_MESH_SIZE 4
#endif

namespace Zephany {

namespace stream_config {
static constexpr unsigned int N = ZEPHANY_MESH_SIZE;
static constexpr unsigned int processors = N * N;
}

enum class stream_direction { up, down };

template <typename T, typename TIdx = Zee::default_index_type>
class Stream {
  public:
    Stream(stream_direction direction) : direction_(direction) {}

    void setChunkSize(TIdx chunkSize) { chunkSize_ = chunkSize; }
    void setTotalSize(TIdx totalSize) { totalSize_ = totalSize; }

    TIdx getChunkSize() const { return chunkSize_; }
    TIdx getTotalSize() const { return totalSize_; }

    std::array<std::vector<T>, stream_config::processors>& getData() {
        return data_;
    }

    void setInitialized() { initialized_ = true; }

    virtual void create() const = 0;

  protected:
    // these are per processor
    TIdx chunkSize_ = 0;
    TIdx totalSize_ = 0;

    // we support upwards and downward streams
    stream_direction direction_ = stream_direction::down;

    std::array<std::vector<T>, stream_config::processors> data_;
    bool initialized_ = false;
};

template <typename T, typename TIdx = Zee::default_index_type>
class UpStream : public Stream<T, TIdx> {
  public:
    UpStream() : Stream<T, TIdx>(stream_direction::up) {}

    void create() const override {}

    void createUp() {
        ZeeAssert(this->chunkSize_ != 0);
        ZeeAssert(this->totalSize_ != 0);

        TIdx totalSize = this->getTotalSize();
        TIdx chunkSize = this->getChunkSize();

        for (TIdx s = 0; s < stream_config::processors; s++) {
            this->rawData_[s] = (T*)ebsp_create_up_stream(s, totalSize, chunkSize);
        }
    }

    const std::array<T*, stream_config::processors>& getRawData() const {
        return rawData_;
    }

  private:
    std::array<T*, stream_config::processors> rawData_;
};


} // namespace Zephany
