#include <stdio.h>
#include <cstdlib>

uint8_t is_alive(uint8_t* neighborhood) {
    uint8_t self;
    uint8_t num_neighbors_alive = 0;
    for (uint8_t i = 0; i < 9; i++) {
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

void update_single_cell(int x, int y, uint8_t* in, uint8_t* out, int width, int height) {
    uint8_t neighborhood[9];
    for (uint8_t i = 0; i < 9; i++) {
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

void simulation_step(uint8_t* in, uint8_t* out, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            update_single_cell(x, y, in, out, width, height);
        }
    }
}

void run_simulation(int iterations, uint8_t* a, uint8_t* b, int width, int height) {
    for (int i = 0; i < iterations; i++) {
        if (i % 2 == 0) {
            simulation_step(a, b, width, height);
        } else {
            simulation_step(b, a, width, height);
        }
    }
}

int main() {
    int height = 1024;
    int width = 1024;
    int len = height * width;
    int iterations = 1000;

    FILE *fptr;
    fptr = fopen("gc_1024x1024-uint8.raw", "rb");
    if (fptr == NULL) {
        printf("Unable to open file.\n");
        return EXIT_FAILURE;
    }

    uint8_t* field_a = (uint8_t*) malloc(len);
    uint8_t* field_b = (uint8_t*) malloc(len);
    int num_read = fread(field_a, 1, len, fptr);
    fclose(fptr);

    run_simulation(iterations, field_a, field_b, width, height);
    uint8_t* output = iterations % 2 == 0 ? field_a : field_b;

    fptr = fopen("gc_out.raw", "wb");
    if (fptr == NULL) {
        printf("Unable to open output file.");
        return EXIT_FAILURE;
    }

    int num_written = fwrite(output, 1, len, fptr);
    fclose(fptr);

    return EXIT_SUCCESS;
}
