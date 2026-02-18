#include <stdio.h>
#include <cstdlib>
#include <cmath>
#include "pthread.h"

struct HistogramArgs {
    float* data;
    float min_meas;
    float max_meas;
    int data_count;
    int bin_count;
    float bin_size;
    int thread_count;
    int thread_data_size;
};

struct PartialGlobalSumArgs {
    HistogramArgs* hist_args;
    int rank;
    int* rank_flag;
    float* bin_maxes;
    int* bin_counts;
    int start;
    int end;
};

struct TreeSumArgs {
    HistogramArgs* hist_args;
    int level;
    int rank;
    float* bin_maxes;
    int* bin_counts;
};

/**
 Reads the given range of data in memory to compute partial histogram counts
 and maximum values, writing output to the provided space in memory.
 */
void compute_partial_histogram(HistogramArgs* args, int start, int end, float* bin_maxes_out, int* bin_counts_out) {
    for (int i = start; i < end; i++) {
        float current = args->data[i];
        int bin = current / args->bin_size;
        if (bin >= args->bin_count) {
            bin = args->bin_count - 1;
        }
        if (bin < 0) {
            continue;
        }
        bin_counts_out[bin]++;
        if (bin_maxes_out[bin] < current) {
            bin_maxes_out[bin] = current;
        }
    }
}

/**
 Combines two partial histograms by adding counts and updating maxes with the
 maximum between both histograms, modifying the output max/count arrays in-place.
 */
void combine_partial_histograms(float* maxes_in, int* counts_in, float* maxes_out, int* counts_out, int bin_count) {
    for (int i = 0; i < bin_count; i++) {
        counts_out[i] += counts_in[i];
        if (maxes_out[i] < maxes_in[i]) {
            maxes_out[i] = maxes_in[i];
        }
    }
}

/**
 Initializes a series of data of provided length and within the provided range.
 */
void init_random_dataset(float* data_out, int data_count, float min_meas, float max_meas) {
    srand(100);
    
    int range = max_meas - min_meas;
    for (int i = 0; i < data_count; i++) {
        data_out[i] = rand() % range + min_meas;
    }
}

/**
 Procedure for a thread that allocates memory for it's own partial histogram,
 computes the histogram, and then waits it's turn to update the global histogram.
 */
void * global_sum_compute_partial(PartialGlobalSumArgs* args) {
    // Initialize local max and count partial histograms
    float bin_maxes_local[args->hist_args->bin_count];
    int bin_counts_local[args->hist_args->bin_count];
    for (int i = 0; i < args->hist_args->bin_count; i++) {
        bin_maxes_local[i] = -1;
        bin_counts_local[i] = 0;
    }
    
    compute_partial_histogram(args->hist_args, args->start, args->end, bin_maxes_local, bin_counts_local);
    
    // Update the global histogram
    while (*args->rank_flag != args->rank);
    combine_partial_histograms(bin_maxes_local, bin_counts_local, args->bin_maxes, args->bin_counts, args->hist_args->bin_count);
    (*args->rank_flag)++;
    
    return NULL;
}

/**
 Launches a given number of threads to compute the complete histogram of a
 dataset using the global sum method.
 */
void hist_global_sum(HistogramArgs* args, float* bin_maxes_out, int* bin_counts_out) {
    void *(*start_routine)(void *) = (void *(*)(void *)) &global_sum_compute_partial;
    
    pthread_t child_threads[args->thread_count];
    PartialGlobalSumArgs child_args[args->thread_count];
    int rank_flag = 0;
    
    // Kick off each thread
    for (int i = 0; i < args->thread_count; i++) {
        child_args[i] = {
            .hist_args = args,
            .rank = i,
            .rank_flag = &rank_flag,
            .bin_maxes = bin_maxes_out,
            .bin_counts = bin_counts_out
        };
        
        // Compute data range for child thread
        child_args[i].start = args->thread_data_size * i;
        if (i == args->thread_count - 1) {
            child_args[i].end = args->data_count;
        } else {
            child_args[i].end = args->thread_data_size * (i + 1);
        }
        
        pthread_create(&child_threads[i], NULL, start_routine, &child_args[i]);
    }
    
    // Wait for all threads to finish
    for (int i = 0; i < args->thread_count; i++) {
        pthread_join(child_threads[i], NULL);
    }
}

/**
 Recursive call that follows the divide-and-conquer approach to launch new threads
 and combine their results.
 */
void * hist_tree_sum_r(TreeSumArgs* args) {
    if (args->level > 0) {
        // Init memory space for child thread
        float child_bin_maxes[args->hist_args->bin_count];
        int child_bin_counts[args->hist_args->bin_count];
        for (int i = 0; i < args->hist_args->bin_count; i++) {
            child_bin_maxes[i] = -1;
            child_bin_counts[i] = 0;
        }
        
        // Launch child thread
        void *(*start_routine)(void *) = (void *(*)(void *)) &hist_tree_sum_r;
        pthread_t child_thread;
        TreeSumArgs child_args = {
            .hist_args = args->hist_args,
            .level = args->level - 1,
            .rank = args->rank + (int) pow(2, args->level - 1),
            .bin_maxes = child_bin_maxes,
            .bin_counts = child_bin_counts
        };
        pthread_create(&child_thread, NULL, start_routine, &child_args);
        
        // Compute one half of the partial histogram
        args->level -= 1;
        hist_tree_sum_r(args);
        
        // Launched thread computes other half; wait for completion
        pthread_join(child_thread, NULL);
        
        // Combine both partial histograms
        combine_partial_histograms(child_bin_maxes, child_bin_counts, args->bin_maxes, args->bin_counts, args->hist_args->bin_count);
    } else {
        // No need to launch a new thread
        // Just compute partial histogram for our portion of the data
        int start = args->hist_args->thread_data_size * args->rank;
        int end;
        if (args->rank == args->hist_args->thread_count - 1) {
            end = args->hist_args->data_count;
        } else {
            end = args->hist_args->thread_data_size * (args->rank + 1);
        }
        
        compute_partial_histogram(args->hist_args, start, end, args->bin_maxes, args->bin_counts);
    }

    return NULL;
}

/**
 Top-level wrapper (entry point) for the tree-structured recursive call. Represents
 the main thread.
 */
void hist_tree_sum(HistogramArgs* args, float* bin_maxes_out, int* bin_counts_out) {
    TreeSumArgs child_args = {
        .hist_args = args,
        .level = (int) log2(args->thread_count),
        .rank = 0,
        .bin_maxes = bin_maxes_out,
        .bin_counts = bin_counts_out
    };
    hist_tree_sum_r(&child_args);
}

void print_data(float* data, int data_count) {
    printf("Data: ");
    for (int i = 0; i < data_count; i++) {
        printf("%f, ", data[i]);
    }
    printf("\n");
}

void print_histogram(float* bin_maxes, int* bin_counts, int bin_count) {
    printf("bin_maxes = ");
    for (int i = 0; i < bin_count; i++) {
        printf("%f ", bin_maxes[i]);
    }
    printf("\n");
    
    printf("bin_counts = ");
    for (int i = 0; i < bin_count; i++) {
        printf("%d ", bin_counts[i]);
    }
    printf("\n");
}

int main(int argc, const char * argv[]) {
//    if (argc != 6) {
//        return 1;
//    }
    
    int data_count = 100;
    float data[data_count];
//    HistogramArgs args = {
//        .data = data,
//        .data_count = atoi(argv[5]),
//        .min_meas = (float) atof(argv[3]),
//        .max_meas = (float) atof(argv[4]),
//        .bin_count = atoi(argv[2]),
//        .thread_count = atoi(argv[1])
//    };
    HistogramArgs args = {
        .data = data,
        .data_count = 105,
        .min_meas = 12,
        .max_meas = 527,
        .bin_count = 10,
        .thread_count = 8
    };
    args.bin_size = (args.max_meas - args.min_meas) / args.bin_count;
    args.thread_data_size = args.data_count / args.thread_count;
    
    init_random_dataset(data, data_count, args.min_meas, args.max_meas);
    print_data(data, data_count);
    
    float bin_maxes_global[args.bin_count];
    int bin_counts_global[args.bin_count];
    for (int i = 0; i < args.bin_count; i++) {
        bin_maxes_global[i] = -1;
        bin_counts_global[i] = 0;
    }
    hist_global_sum(&args, bin_maxes_global, bin_counts_global);
    printf("Global Sum\n");
    print_histogram(bin_maxes_global, bin_counts_global, args.bin_count);
    
//    float bin_maxes_tree[args.bin_count];
//    int bin_counts_tree[args.bin_count];
//    for (int i = 0; i < args.bin_count; i++) {
//        bin_maxes_tree[i] = -1;
//        bin_counts_tree[i] = 0;
//    }
//    hist_tree_sum(&args, bin_maxes_tree, bin_counts_tree);
//    printf("Tree Structured Sum\n");
//    print_histogram(bin_maxes_tree, bin_counts_tree, args.bin_count);
}
