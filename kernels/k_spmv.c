#include <e_bsp.h>
#include <stdint.h>

int main()
{
    // we use double buffered mode
    const int double_buffer = 1;

    // there is a single down stream containing the relevant information
    uint32_t* chunk = NULL;
    ebsp_open_down_stream((void**)&chunk, 0);
    ebsp_move_chunk_down((void**)&chunk, 0, double_buffer);

    // the first chunk contains the header
    uint32_t max_size_u = chunk[0];
    uint32_t max_size_v = chunk[1];
    uint32_t max_size_window = chunk[2];
    uint32_t max_non_local = chunk[3];
    uint32_t num_strips = chunk[4];

    for (uint32_t strip = 0; strip < num_strips; ++strip) {
        // next chunk contains strip header
        ebsp_move_chunk_down((void**)&chunk, 0, double_buffer);

        // next follow a number of window chunks
        // send result up
    }

    // we close the down stream
    ebsp_close_down_stream(0);

    return 0;
}
