#include "conway.cpp"
#include <time.h>

void simulation_step(u_int8_t* in, u_int8_t* out, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            update_single_cell(x, y, in, out, width, height);
        }
    }
}

void run_simulation(int iterations, u_int8_t* a, u_int8_t* b, int width, int height) {
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

    u_int8_t* field_a = (u_int8_t*) malloc(len);
    u_int8_t* field_b = (u_int8_t*) malloc(len);
    int num_read = fread(field_a, 1, len, fptr);
    fclose(fptr);

    struct timespec start_time;
    struct timespec end_time;

    clock_gettime(CLOCK_REALTIME, &start_time);

    run_simulation(iterations, field_a, field_b, width, height);
    u_int8_t* output = iterations % 2 == 0 ? field_a : field_b;

    clock_gettime(CLOCK_REALTIME, &end_time);

    double time_ns = (double) (end_time.tv_sec - start_time.tv_sec) * 1.0e9 + (double) (end_time.tv_nsec - start_time.tv_nsec);
    double time_ms = time_ns / 1000000;
    printf("Finished in %f ns\n", time_ms);

    fptr = fopen("gc_out-serial.raw", "wb");
    if (fptr == NULL) {
        printf("Unable to open output file.");
        return EXIT_FAILURE;
    }

    int num_written = fwrite(output, 1, len, fptr);
    fclose(fptr);

    return EXIT_SUCCESS;
}