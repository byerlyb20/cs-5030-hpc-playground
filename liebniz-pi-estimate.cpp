//
//  liebniz-pi-estimate.cpp
//  hpc-playground
//
//  Created by Brigham Byerly on 1/28/26.
//

#include <mach/mach_time.h>
#include <iostream>
#include <cstdio>
#include "pthread.h"

struct LiebnizSeriesArgs {
    long start;  // inclusive
    long end;    // exclusive
    double* out;
};

void * liebniz_series_compute(LiebnizSeriesArgs * data_in) {
    *data_in->out = 0.0;
    for (long i = data_in->start; i < data_in->end; i++) {
        double term = 1.0/(2*i+1);
        if (i & 1) {
            term *= -1;
        }
//        printf("Iter #%d: %f\n", i, term);
        *data_in->out += term;
    }
    return NULL;
}

int main(int argc, const char * argv[]) {
    uint64_t start = mach_absolute_time();
    void *(*start_routine)(void *) = (void *(*)(void *)) &liebniz_series_compute;
    
    int num_threads = 10;
    long num_iterations = 1e13;
    long iter_per_thread = num_iterations / num_threads;
    
    LiebnizSeriesArgs child_thread_args[num_threads];
    pthread_t child_threads[num_threads];
    double results[num_threads];
    double result = 0.0;
    
    // Kick off all threads
    for (int i = 0; i < num_threads; i++) {
        child_thread_args[i] = {
            .start = i*iter_per_thread,
            .end = i*iter_per_thread+iter_per_thread,
            .out = &results[i]
        };
        pthread_create(&child_threads[i], NULL, start_routine, &child_thread_args[i]);
    }
    
    // Wait for all threads to finish and sum results
    for (int i = 0; i < num_threads; i++) {
        pthread_join(child_threads[i], NULL);
        result += results[i];
    }
    
    result *= 4;
    uint64_t end = mach_absolute_time();
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    uint64_t elapsed_ms = ((end-start) * timebase.numer) / timebase.denom / 1e6;
    printf("Final answer: %.20f\n", result);
    printf("Runtime (ms): %llu\n", elapsed_ms);
        
    return EXIT_SUCCESS;
}
