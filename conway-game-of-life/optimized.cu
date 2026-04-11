#include <stdlib.h>
#include <stdio.h>
#include "conway.cpp"

#define TILE_SIZE 32;

__global__ void conway_step_kernel(u_int8_t* in, u_int8_t* out, int width, int height) {
    __shared__ u_int8_t tile[TILE_SIZE + 2][TILE_SIZE + 2];

    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;

    // Each thread copies its own cell
    tile[blockIdx.y+1][blockIdx.x+1] = in[y*width + x];

    int numbering = blockIdx.y * TILE_SIZE + blockIdx.x;
    int halo_tile_x, halo_tile_y;
    if (numbering < TILE_SIZE+2) {
        halo_tile_y = 0;
        halo_tile_x = numbering;
    } else {
        numbering -= TILE_SIZE+2;
        if (numbering < TILE_SIZE+2) {
            halo_tile_y = TILE_SIZE + 1;
            halo_tile_x = numbering;
        } else {
            numbering -= TILE_SIZE+2;
            if (numbering < TILE_SIZE) {
                halo_tile_y = numbering + 1;
                halo_tile_x = 0;
            } else {
                numbering -= TILE_SIZE;
                if (numbering < TILE_SIZE) {
                    halo_tile_y = numbering + 1;
                    halo_tile_x = TILE_SIZE + 2;
                }
            }
        }
    }
    int halo_x = blockIdx.x * blockDim.x - 1 + halo_tile_x;
    int halo_y = blockIdx.y * blockDim.y - 1 + halo_tile_y;
    if (halo_x < width && halo_y < height) {
        tile[halo_tile_y][halo_tile_x] = in[halo_y*width + halo_x];
    }

    // Corners are not copied

    __syncthreads();

    if (x < width && y < height) {
        u_int8_t neighborhood[9];
        for (u_int8_t i = 0; i < 9; i++) {
            int neighbor_x = (i % 3) - 1 + x;
            int neighbor_y = (i / 3) - 1 + y;
            int neighbor_tile_x = (i % 3) + blockIdx.x;
            int neighbor_tile_y = (i / 3) + blockIdx.y;
            if (neighbor_x > 0 && neighbor_x < width && neighbor_y > 0 && neighbor_y < height) {
                neighborhood[i] = tile[neighbor_tile_y][neighbor_tile_x];
            } else {
                neighborhood[i] = false;
            }
        }
        out[y*width + x] = is_alive(neighborhood);
    }
}

int main() {
    int height = 1024;
    int width = 1024;
    int len = height * width;
    int iterations = 1000;
    int block_size_1d = TILE_SIZE;

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

    dim3 blocksPerGrid(ceil(len/(double)block_size_1d), ceil(len/(double)block_size_1d)); 
    dim3 threadsPerBlock(block_size_1d, block_size_1d);

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
        printf("Ran %d-th simulation\n", i);
    }

    u_int8_t* output_d = iterations % 2 == 0 ? field_a_d : field_b_d;
    ret = cudaMemcpy(field_h, output_d, len, cudaMemcpyDeviceToHost);
    if (ret != cudaSuccess) {
        fprintf(stderr, "Copying memory from device to host failed.\n");
        return EXIT_FAILURE;
    }

    fptr = fopen("gc_out.raw", "wb");
    if (fptr == NULL) {
        printf("Unable to open output file.");
        return EXIT_FAILURE;
    }

    int num_written = fwrite(field_h, 1, len, fptr);
    fclose(fptr);

    return EXIT_SUCCESS;
}
