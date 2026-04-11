# Conway's Game of Life Simulation
## Runtime

* Serial: 133290.477983 ms
* Naive CUDA: 32.311710 ms
* Optimized CUDA: 41.148865 ms

## Effective Memory Bandwidth for GPU versions

Each step of the simulation performs roughly 1024x1024x9=9437184 memory
accesses and 1024x1024=1048576 memory writes, each one byte each for a total of
0.01 GB. Thus, we have:

* Naive CUDA: 10.48576/0.032 = 328 GB/s
* Optimized CUDA: 10.48576/0.041 = 256 GB/s
