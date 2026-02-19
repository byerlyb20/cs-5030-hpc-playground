#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int num_threads = 1;

void print_array(int* a, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", a[i]);
    }
    printf("\n");
}

void count_sort(int a[], int n) {
    int* sorted = (int*) malloc(n * sizeof(int));

    // Spawn num_threads new threads
    #pragma omp parallel num_threads(num_threads)
    {

        // Split iterations of the for loop amongst available threads
        #pragma omp for
        for (int i = 0; i < n; i++) {
            int count = 0;
            for (int j = 0; j < n; j++) {
                if (a[j] < a[i]) {
                    count++;
                } else if (a[j] == a[i] && j < i) {
                    count++;
                }
            }
            sorted[count] = a[i];
        }

        // Copy each element one at a time; this approach is naive but
        // demonstrates an alternative to memcpy
        #pragma omp for
        for (int i = 0; i < n; i++) {
            a[i] = sorted[i];
        }
    }

    free(sorted);
} /* count_sort */

int main(int argc, const char * argv[]) {
    srand(100);

    int size = atoi(argv[2]);
    num_threads = atoi(argv[1]);

    // Randomly initialize the array on the heap to support very large arrays
    int* array = (int*) malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        array[i] = (rand() % 100) + 1;
    }

    printf("original: \t");
    print_array(array, size);

    count_sort(array, size);

    printf("sorted: \t");
    print_array(array, size);

    free(array);
}
