#include "conway.cpp"

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

    run_simulation(iterations, field_a, field_b, width, height);
    u_int8_t* output = iterations % 2 == 0 ? field_a : field_b;

    fptr = fopen("gc_out.raw", "wb");
    if (fptr == NULL) {
        printf("Unable to open output file.");
        return EXIT_FAILURE;
    }

    int num_written = fwrite(output, 1, len, fptr);
    fclose(fptr);

    return EXIT_SUCCESS;
}