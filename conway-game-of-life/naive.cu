#include "conway.cpp"

__global__ void conway_step_kernel(uint8_t* in, uint8_t* out, int width, int height) {
    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;
    
    update_single_cell(x, y, in, out, width, height);
}

int main() {
    return EXIT_SUCCESS;
}
