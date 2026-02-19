## Homework Questions

1. **If we try to parallelize the for i loop (the outer loop), which variables should be private and which should be shared?**
   
   Given the sample code and assuming only the outer loop is parallelized, `j` and `count` need to be explicitly made private. Since `i` is a loop control variable for a parallelized loop, OpenMP will automatically make `i` private.

2. **If we consider the _memcpy_ implementation not thread-safe, how would you approach parallelizing this operation?**

   Given the sample code and assuming only the outer loop is parallelized, `memcpy` is safe to call even if the _memcpy_ implementation is not thread-safe. Since this call is made outside the parallelized for loop, only a single thread will call `memcpy` and access the memory regions involved in the copy. If we needed to use `memcpy` across multiple threads and the `memcpy` implementation was not known to be thread-safe, we could configure OpenMP to enforce a critical section using the `omp critical` directive. This way, only one thread would make a call to `memcpy` at a time.

### For my reference

After installing `gcc` via brew on Mac (system copy of clang does not support OpenMP), compile as follows:

```
gcc-15 count_sort.c -fopenmp -o out/count_sort && chmod +x ./out/count_sort
```

To demonstrate that threading is working properly, compare the runtime of the following:

```
time out/count_sort 10 100000
```

```
time out/count_sort 1 100000
```

Respective runtimes on my machine: 5s, 35s