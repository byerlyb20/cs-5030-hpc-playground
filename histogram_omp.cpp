#include <stdio.h>
#include <cstdlib>
#include <cmath>
#include "pthread.h"
#include <mpi.h>

/**
 Reads the given range of data in memory to compute partial histogram counts,
 writing output to the provided space in memory.
 */
void compute_partial_histogram(float* data, int data_count, int bin_count, float bin_size, int* bin_counts_out) {
    for (int i = 0; i < data_count; i++) {
        float current = data[i];
        int bin = current / bin_size;
        if (bin >= bin_count) {
            bin = bin_count - 1;
        }
        if (bin < 0) {
            continue;
        }
        bin_counts_out[bin]++;
    }
}

/**
 Combines two partial histograms by adding counts and updating maxes with the
 maximum between both histograms, modifying the output max/count arrays in-place.
 */
void combine_partial_histograms(int* counts_in, int* counts_out, int bin_count) {
    for (int i = 0; i < bin_count; i++) {
        counts_out[i] += counts_in[i];
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
    int comm_size;
    int rank;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   if (argc != 5) {
       return 1;
   }

    int bin_count;
    float bin_size;

    if (rank == 0) {
        int master_data_count = atoi(argv[4]);
        float master_data[master_data_count];
        
        float min_meas = (float) atof(argv[2]);
        float max_meas = (float) atof(argv[3]);
        bin_count = atoi(argv[1]);
        bin_size = (max_meas - min_meas) / bin_count;
        int proc_data_size = master_data_count / comm_size;

        MPI_Bcast(&bin_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&bin_size, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
        
        init_random_dataset(master_data, master_data_count, min_meas, max_meas);
        print_data(master_data, master_data_count);

        int offsets[comm_size];
        int data_len[comm_size];

        int offset_cursor = 0;
        for (int i = 0; i < (comm_size - 1); i++) {
            offsets[i] = offset_cursor;
            data_len[i] = proc_data_size;
            offset_cursor += proc_data_size;
        }

        // Last process takes the remaining data regardless (might be more or
        // less than other processes)
        offsets[comm_size - 1] = offset_cursor;
        data_len[comm_size - 1] = master_data_count - offset_cursor;

        // Distribute the entire dataset in chunks
        int data_count;
        MPI_Scatter(data_len, 1, MPI_INT, &data_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
        float data[data_count];
        MPI_Scatterv(master_data, data_len, offsets, MPI_FLOAT, &data, data_count, MPI_FLOAT, 0, MPI_COMM_WORLD);

        // Compute partial histogram for chunk assigned to this process
        int partial_histogram[bin_count];
        for (int i = 0; i < bin_count; i++) {
            partial_histogram[i] = 0;
        }
        compute_partial_histogram(data, data_count, bin_count, bin_size, partial_histogram);

        // Combine partial histograms
        float maxes[bin_count];
        int histogram[bin_count];
        for (int i = 0; i < bin_count; i++) {
            maxes[i] = bin_size * (i+1);
            histogram[i] = 0;
        }
        MPI_Reduce(&partial_histogram, &histogram, bin_count, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        // Print results
        printf("OpemMPI Multi-process\n");
        print_histogram(maxes, histogram, bin_count);
    } else {
        MPI_Bcast(&bin_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&bin_size, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

        // Receive a chunk of the master dataset
        int data_count;
        MPI_Scatter(NULL, 1, MPI_INT, &data_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
        float data[data_count];
        MPI_Scatterv(NULL, NULL, NULL, MPI_FLOAT, &data, data_count, MPI_FLOAT, 0, MPI_COMM_WORLD);

        // Compute partial histogram over assigned chunk
        int partial_histogram[bin_count];
        for (int i = 0; i < bin_count; i++) {
            partial_histogram[i] = 0;
        }
        compute_partial_histogram(data, data_count, bin_count, bin_size, partial_histogram);

        // Combine partial histograms
        MPI_Reduce(&partial_histogram, NULL, bin_count, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
