#include <stdlib.h>
#include <stdio.h>
#include "conway.cpp"
#include <time.h>

__global__ void conway_step_kernel(u_int8_t* in, u_int8_t* out, int width, int height) {
    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;

    if (x < width && y < height) {
        update_single_cell(x, y, in, out, width, height);
    }
}

int main() {
    int height = 1024;
    int width = 1024;
    int len = height * width;
    int iterations = 1000;
    int block_size_1d = 32;

    FILE *fptr;
    fptr = fopen("gc_1024x1024-uint8.raw", "rb");
    if (fptr == NULL) {
        printf("Unable to open file.\n");
        return EXIT_FAILURE;
    }

    u_int8_t* field_h = (u_int8_t*) malloc(len);
    int num_read = fread(field_h, 1, len, fptr);
    fclose(fptr);

    cudaError_t ret;

    u_int8_t* field_a_d;
    u_int8_t* field_b_d;

    ret = cudaMalloc((void **) &field_a_d, len);
    if (ret != cudaSuccess) {
        fprintf(stderr, "Device memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    ret = cudaMalloc((void **) &field_b_d, len);
    if (ret != cudaSuccess) {
        fprintf(stderr, "Device memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    ret = cudaMemcpy(field_a_d, field_h, len, cudaMemcpyHostToDevice);
    if (ret != cudaSuccess) {
        fprintf(stderr, "Copying memory from host to device failed.\n");
        return EXIT_FAILURE;
    }

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    dim3 blocksPerGrid(ceil(width/(double)block_size_1d), ceil(height/(double)block_size_1d)); 
    dim3 threadsPerBlock(block_size_1d, block_size_1d);

    cudaEventRecord(start);

    for (int i = 0; i < iterations; i++) {
        if (i % 2 == 0) {
            conway_step_kernel<<<blocksPerGrid, threadsPerBlock>>>(field_a_d, field_b_d, width, height);
        } else {
            conway_step_kernel<<<blocksPerGrid, threadsPerBlock>>>(field_b_d, field_a_d, width, height);
        }
        ret = cudaDeviceSynchronize();
        if (ret != cudaSuccess) {
            fprintf(stderr, "CUDA synchronize failed.\n");
            return EXIT_FAILURE;
        }
    }

    cudaEventRecord(stop);

    ret = cudaEventSynchronize(stop);
    if (ret != cudaSuccess) {
        fprintf(stderr, "CUDA synchronize failed with ret=%d\n", ret);
        return EXIT_FAILURE;
    }

    float time_ms = 0;
    cudaEventElapsedTime(&time_ms, start, stop);
    printf("Finished in %f ms\n", time_ms);

    u_int8_t* output_d = iterations % 2 == 0 ? field_a_d : field_b_d;
    ret = cudaMemcpy(field_h, output_d, len, cudaMemcpyDeviceToHost);
    if (ret != cudaSuccess) {
        fprintf(stderr, "Copying memory from device to host failed.\n");
        return EXIT_FAILURE;
    }

    fptr = fopen("gc_out-naive.raw", "wb");
    if (fptr == NULL) {
        printf("Unable to open output file.");
        return EXIT_FAILURE;
    }

    int num_written = fwrite(field_h, 1, len, fptr);
    fclose(fptr);

    return EXIT_SUCCESS;
}
