#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

__global__ void rgb_to_grayscale_kernel(char* rgb_im_in, char* grayscale_im_out, int size) {
    // thread id of current block (on x axis)
    // int tid = threadIdx.x;

    // block id (on x axis)
    // int bx = blockIdx.x;

    int pixel_idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (pixel_idx < size) {
        char r = rgb_im_in[pixel_idx * 3];
        char g = rgb_im_in[pixel_idx * 3 + 1];
        char b = rgb_im_in[pixel_idx * 3 + 2];

        // Equation taken from Lecture 17: CUDA Execution Model
        grayscale_im_out[pixel_idx] = 0.21f*r + 0.71f*g + 0.07f*b;
    }
}

int main() {
    int height = 1024;
    int width = 1024;
    int block_size = 1024;
    int rgb_im_len = height * width * 3;
    int grayscale_im_len = height * width;

    // Read file into host memory

    FILE *fptr;
    fptr = fopen("gc_conv_1024x1024.raw", "rb");
    if (fptr == NULL) {
        printf("Unable to open file.\n");
        return EXIT_FAILURE;
    }

    char* rgb_im_h = (char*) malloc(rgb_im_len);
    int num_read = fread(rgb_im_h, 1, rgb_im_len, fptr);
    fclose(fptr);

    printf("Read %d bytes\n", num_read);

    // Allocate memory and copy file to device

    cudaError_t ret;

    char* rgb_im_d;
    char* grayscale_im_d;

    ret = cudaMalloc((void **) &rgb_im_d, rgb_im_len);
    if (ret != cudaSuccess) {
        fprintf(stderr, "Device memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    ret = cudaMalloc((void **) &grayscale_im_d, grayscale_im_len);
    if (ret != cudaSuccess) {
        fprintf(stderr, "Device memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    ret = cudaMemcpy(rgb_im_d, rgb_im_h, rgb_im_len, cudaMemcpyHostToDevice);
    if (ret != cudaSuccess) {
        fprintf(stderr, "Copying memory from host to device failed.\n");
        return EXIT_FAILURE;
    }

    // Free host copy of image

    free(rgb_im_h);

    // Run grayscale kernel 200x and take the average runtime

    struct timespec start_time;
    struct timespec end_time;
    double avg_time = 0;
    int num_trials = 200;

    for (int i = 0; i < num_trials; i++) {
        clock_gettime(CLOCK_REALTIME, &start_time);

        rgb_to_grayscale_kernel<<<ceil(rgb_im_len/block_size), block_size>>>(rgb_im_d, grayscale_im_d, grayscale_im_len);

        ret = cudaDeviceSynchronize();
        if (ret != cudaSuccess) {
            fprintf(stderr, "Grayscale kernel failed.\n");
            return EXIT_FAILURE;
        }

        clock_gettime(CLOCK_REALTIME, &end_time);

        double time_ns = (double) (end_time.tv_sec - start_time.tv_sec) * 1.0e9 + (double) (end_time.tv_nsec - start_time.tv_nsec);
        avg_time += time_ns;
    }

    avg_time /= num_trials;

    printf("Completed %d trials in an average of %f ns\n", num_trials, avg_time);

    // Copy results back to host

    char* grayscale_im_h = (char*) malloc(grayscale_im_len);

    ret = cudaMemcpy(grayscale_im_h, grayscale_im_d, grayscale_im_len, cudaMemcpyDeviceToHost);
    if (ret != cudaSuccess) {
        fprintf(stderr, "Copying memory from device to host failed.\n");
        return EXIT_FAILURE;
    }

    // Write results to file

    fptr = fopen("gc.raw", "wb");
    if (fptr == NULL) {
        printf("Unable to open output file.");
        return EXIT_FAILURE;
    }

    int num_written = fwrite(grayscale_im_h, 1, grayscale_im_len, fptr);
    fclose(fptr);

    printf("Wrote %d bytes\n", num_written);

    free(grayscale_im_h);

    return 0;
}