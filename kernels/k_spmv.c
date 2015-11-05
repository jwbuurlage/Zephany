#include <e_bsp.h>
#include <stdint.h>

typedef uint32_t uint;

int main() {
    // we use double buffered mode
    const int double_buffer = 1;

    // there is a single down stream containing the relevant information
    uint* chunk = NULL;
    ebsp_open_down_stream((void**)&chunk, 0);
    ebsp_move_chunk_down((void**)&chunk, 0, double_buffer);

    // the first chunk contains the header
    //uint max_size_u = chunk[0]; // FIXME obsolete
    uint max_size_v = chunk[1];
    //uint max_size_window = chunk[2]; // FIXME obsolete
    uint max_non_local = chunk[3];
    uint num_strips = chunk[4];

    uint* v = ebsp_malloc((max_size_v + max_non_local) * sizeof(float));
    uint* u = NULL;
    ebsp_open_up_stream((void**)&u, 1);

    bsp_push_reg(v, sizeof(uint) * max_non_local + max_size_v);
    bsp_sync();

    for (uint strip = 0; strip < num_strips; ++strip) {
        // next chunk contains strip header
        ebsp_move_chunk_down((void**)&chunk, 0, double_buffer);
        uint num_windows = chunk[0];
        uint num_local_v = chunk[1];

        // we copy the strip v's to the proper location
        ebsp_memcpy(v, (uint*)&chunk[2], sizeof(uint) * num_local_v);

        // next follow a number of window chunks
        for (uint window = 0; window < num_windows; ++window) {
            ebsp_move_chunk_down((void**)&chunk, 0, double_buffer);

            // we maintain the current location in the window chunk
            uint cursor = 0;

            uint num_non_local = chunk[cursor++];

            uint* non_local_owners = &chunk[cursor];
            cursor += num_non_local;

            uint* non_local_idxs = &chunk[cursor];
            cursor += num_non_local;

            // obtain non local v's
            for (uint idx = 0; idx < num_non_local; ++idx) {
                // we obtain v[num_local_v + idx] from non_local_owners[idx]
                // at non_local_idxs[idx]
                bsp_hpget(non_local_owners[idx], v, non_local_idxs[idx],
                          &v[num_local_v + idx], sizeof(float));
            }
            ebsp_barrier();

            uint size_u = chunk[cursor++];
            uint window_size = chunk[cursor++];

            uint* triplet_rows = &chunk[cursor];
            cursor += window_size;

            uint* triplet_cols = &chunk[cursor];
            cursor += window_size;

            float* triplet_vals = (float*)&chunk[cursor];

            // we compute the products
            for (uint idx = 0; idx < window_size; ++idx) {
                u[triplet_rows[idx]] = v[triplet_cols[idx]] * triplet_vals[idx];
            }

            // send result up
            ebsp_set_up_chunk_size(1, sizeof(uint) * size_u);
            ebsp_move_chunk_up((void**)&u, 1, double_buffer);
        }

    }

    // we close the down stream
    ebsp_close_down_stream(0);
    ebsp_close_up_stream(1);

    ebsp_free(v);

    return 0;
}
