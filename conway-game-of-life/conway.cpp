#include <stdio.h>
#include <cstdlib>

#ifdef __CUDACC__
    #define HOST_DEVICE __host__ __device__
#else
    #define HOST_DEVICE
#endif

HOST_DEVICE u_int8_t is_alive(u_int8_t* neighborhood) {
    u_int8_t self;
    u_int8_t num_neighbors_alive = 0;
    for (u_int8_t i = 0; i < 9; i++) {
        if (i == 4) {
            self = neighborhood[i];
            continue;
        }
        if (neighborhood[i]) num_neighbors_alive++;
    }

    if (self) {
        // A live cell with fewer than 2 live neighbors dies
        // A live cell with 2 or 3 neighbors survives
        // A live cell with more than 3 neighbors dies
        return num_neighbors_alive == 2 || num_neighbors_alive == 3;
    } else {
        // A dead cell with exactly 3 (alive) neighbors becomes alive
        return num_neighbors_alive == 3;
    }
}

HOST_DEVICE void update_single_cell(int x, int y, u_int8_t* in, u_int8_t* out, int width, int height) {
    u_int8_t neighborhood[9];
    for (u_int8_t i = 0; i < 9; i++) {
        int neighbor_x = (i % 3) - 1 + x;
        int neighbor_y = (i / 3) - 1 + y;
        if (neighbor_x > 0 && neighbor_x < width && neighbor_y > 0 && neighbor_y < height) {
            neighborhood[i] = in[neighbor_y*width + neighbor_x];
        } else {
            neighborhood[i] = false;
        }
    }
    out[y*width + x] = is_alive(neighborhood);
}
