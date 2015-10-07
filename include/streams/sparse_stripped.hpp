#pragma once

#include "streams.hpp"

/* STREAM DEFINITION
 * =================
 * - max_u_size        int
 * - max_v_size        int
 * - max_block_size    int
 * - max_strip_width   int
 * - num_strips        int
 * strip header
 * - num_windows       int
 * window header
 * - num_nonlocal      int                  (or once max_nonlocals)
 * - nonlocal_owners   int[num_nonlocal]
 * - nonlocal_idxs     int[num_nonlocal]
 * - window_size       int
 * window (TODO: crs)
 * triplet_rows        int[window_size]
 * triplet_cols        int[window_size]
 * triplet_vals        float[window_size]
 *
 * LOCAL STORAGE
 * =============
 * A in use
 * A next
 * remote owners
 * remote indices
 * v local
 * v from remote
 * u (always local)
 */

namespace Zephany {
template <typename T, typename TIdx = Zee::default_index_type>
class SparseStrippedStream : public Stream<T, TIdx> {
  public:
    constexpr static int windowDimension = 50;

    SparseStrippedStream()
        : Stream<T, TIdx>(stream_direction::down) {}

    void create() const override {
        // TODO implement
    }

  private:
};

} // namespace Zephany
