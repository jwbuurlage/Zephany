#include <e_bsp.h>
#include <stdint.h>
#include "common.h"

void get_initial_data(void* A, void* B);
void get_parameters(int* inner_block_size, int* outer_blocks, int* N);
void matrix_multiply_add(float* A, float* B, float* C, int inner_block_size);
void ebsp_set_head(int, int);

int main() {
    bsp_begin();

    // Obtain Cannon parameters from the host
    int inner_block_size = 0;
    int outer_blocks = 0;
    int N = 0;
    get_parameters(&inner_block_size, &outer_blocks, &N);
    int inner_block_bytes = inner_block_size * inner_block_size * sizeof(float);

    ebsp_message("Params: %i %i %i %i", inner_block_size, outer_blocks, N,
                 inner_block_bytes);

    // Compute mesh position of this processor
    int s = bsp_pid();
    int si = s / N;
    int sj = s % N;

    // We compute the processor IDs of our neighbours in the mesh
    int a_neighbor = si * N + ((sj + 1) % N);
    int b_neighbor = ((si + 1) % N) * N + sj;

    // We define 5 buffers to hold the matrix blocks
    float* a_data[2];
    float* b_data[2];
    // FIXME = 0
    float* c_data = ebsp_malloc(inner_block_bytes);

    // Whether we want to use double-buffering for C
    const int fastmode = 0;

    ebsp_message("%i", __LINE__);
    // Allocate local buffers
    ebsp_open_down_stream((void**)&a_data[0], 0);
    a_data[1] = ebsp_malloc(inner_block_bytes);
    ebsp_open_down_stream((void**)&b_data[0], 1);
    b_data[1] = ebsp_malloc(inner_block_bytes);

    //ebsp_open_up_stream((void**)&c_data, 2);

    // Set C to zero
    for (int i = 0; i < inner_block_size * inner_block_size; ++i) {
        c_data[i] = 0;
    }

    // Register the locations of our buffers
    bsp_push_reg(a_data[0], inner_block_bytes);
    bsp_sync();
    bsp_push_reg(a_data[1], inner_block_bytes);
    bsp_sync();
    bsp_push_reg(b_data[0], inner_block_bytes);
    bsp_sync();
    bsp_push_reg(b_data[1], inner_block_bytes);
    bsp_sync();
    ebsp_message("%i", __LINE__);

    // We store our neighbor's buffer locations
    float* neighbor_a_data[2];
    float* neighbor_b_data[2];

    // Obtain neighbor locations
    neighbor_a_data[0] = ebsp_get_raw_address(a_neighbor, a_data[0]);
    neighbor_a_data[1] = ebsp_get_raw_address(a_neighbor, a_data[1]);
    neighbor_b_data[0] = ebsp_get_raw_address(b_neighbor, b_data[0]);
    neighbor_b_data[1] = ebsp_get_raw_address(b_neighbor, b_data[1]);

    // We use the DMA manually to send the inner matrix blocks to our neighbours
    ebsp_dma_handle dma_handle_a;
    ebsp_dma_handle dma_handle_b;

    ebsp_message("%i", __LINE__);

    // Make sure the host is awake. FIXME temporary
    ebsp_host_sync();
    ebsp_barrier();

    ebsp_message("%i", __LINE__);

    // Loop over the outer blocks (chunks)
    int total_block_count = outer_blocks * outer_blocks * outer_blocks;
    for (int cur_block = 0; cur_block <= total_block_count; cur_block++) {
        // See if we have something to send up
        if (cur_block != 0) {
            if (cur_block % (outer_blocks * outer_blocks) == 0) {
                ebsp_move_down_cursor(1,         // stream id
                                      -(outer_blocks * outer_blocks)); // relative chunk count
            } else if (cur_block % outer_blocks == 0) {
                ebsp_move_down_cursor(0,   // stream id
                                      -outer_blocks); // relative chunk count
            }
            if (cur_block % outer_blocks == 0) {
                // Send result of C upwards
                // FIXME THIS SHOULD GIVE ERROR IF 2 DOES NOT EXIST
                // ebsp_move_chunk_up((void*)&c_data, 2, fastmode);
                ebsp_message("%i (%i, %i, %i, ..., %i)",
                     cur_block, (int)c_data[0],
                     (int)c_data[1],
                     (int)c_data[2],
                     (int)c_data[CORE_BLOCK_SIZE * CORE_BLOCK_SIZE - 1]);
                ebsp_barrier();

                // FIXME find more elegant way of accomplishing this.
                if (cur_block == total_block_count) {
                    break;
                }

                // Set C to zero
                for (int i = 0; i < inner_block_size * inner_block_size; ++i)
                    c_data[i] = 0;
            }
        }

        // Obtain the inner blocks A_i, B_i
        ebsp_move_chunk_down((void**)&a_data[0], // address
                             0,                  // stream id
                             0);                 // double buffered mode
        ebsp_move_chunk_down((void**)&b_data[0], // address
                             1,                  // stream id
                             0);                 // double buffered mode

        // Define indices into our buffers
        int cur = 0;        // computation
        int cur_buffer = 1; // data transfer

        // Multiply this block, by looping over the *inner blocks*
        for (int i = 0; i < N; ++i) {
            if (i != N - 1) {
                ebsp_dma_push(&dma_handle_a, neighbor_a_data[cur_buffer],
                              a_data[cur], inner_block_bytes);
                ebsp_dma_push(&dma_handle_b, neighbor_b_data[cur_buffer],
                              b_data[cur], inner_block_bytes);
            }

            // Perform C += A * B
            matrix_multiply_add(a_data[cur], b_data[cur], c_data, inner_block_size);

            if (i == N - 1)
                break;

            // Switch buffers
            cur_buffer = 1 - cur_buffer;
            cur = 1 - cur;

            // FIXME: since we use memcpy instead of dma we commented this out
            // ebsp_dma_wait(&dma_handle_a);
            // ebsp_dma_wait(&dma_handle_b);

            // Make sure every dma transfer is finished
            ebsp_barrier();
        }
    }

    ebsp_close_down_stream(0);
    ebsp_close_down_stream(1);
    //ebsp_close_up_stream(2);

    bsp_end();
}

void get_parameters(int* inner_block_size, int* outer_blocks, int* N) {
    int packets = 0;
    int accum_bytes = 0;
    int status = 0;
    int tag = 0;

    bsp_qsize(&packets, &accum_bytes);
    for (int i = 0; i < packets; ++i) {
        bsp_get_tag(&status, &tag);
        if (tag == 0) {
            bsp_move(inner_block_size, sizeof(int));
        } else if (tag == 1) {
            bsp_move(outer_blocks, sizeof(int));
        } else if (tag == 2) {
            bsp_move(N, sizeof(int));
        }
    }
}

// TODO: assembly
void matrix_multiply_add(float* A, float* B, float* C, int inner_block_size) {
    for (int i = 0; i < inner_block_size; ++i)
        for (int j = 0; j < inner_block_size; j++)
            for (int k = 0; k < inner_block_size; k++)
                C[i * inner_block_size + j] +=
                    A[i * inner_block_size + k] * B[k * inner_block_size + j];
}
