# RGB to grayscale CUDA utility
## Usage

Compiled on CHPC notchpeak-shared-short using
```
nvcc grayscale.cu -o grayscale
```
with CUDA 11.6. Simply run:
```
./grayscale
```
in a working directory with a copy of the `gc_conv_1024x1024.raw` image.

## Runtime analysis

Tested and working on CHPC `notchpeak-shared-short` with __NVIDIA GeForce GTX 1080 Ti__ (max. number of threads per SM is 2048).

The average kernel runtime (memory copy not included) was taken over 200 trials for the following block sizes:

1. 1024: 34056 ns
2. 1000: 35081 ns
3. 768: 39363 ns

A block size of 1024 is theoretically ideal and this posture is supported by the results above. With a block size of 1024, a single streaming multiprocessor (SM) can fit two blocks at one time. Additionally, a single block divides evenly across 32 warps, leading to no extra (unused) threads.

If we select a block size of 1000, a single SM is still able to fit two blocks at a time. However, the last warp in each block only uses 8 of 32 possible threads, leaving 24 unused threads.

If we select a block size of 768, a single SM is still only able to fit two blocks at a time. However, only 1536 of a possible 2048 threads are used, leaving 16 unused warps (512 unused threads).

Just for fun, I also tried running with a block size of 512, which also fits both the data and the GPU architecture evenly and should maximize use of the available hardware. This configuration actually beat all of the above configurations, running in an average of 31422 ns.