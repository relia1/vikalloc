#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vikalloc.h"

#define NUM_ITERATIONS 1000000
#define SIZE 32

void benchmark_vikalloc() {
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        void *ptr = vikalloc(SIZE);
        vikfree(ptr);
    }
    end = clock();

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("vikalloc time: %f seconds\n", cpu_time_used);
}

void benchmark_malloc() {
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        void *ptr = malloc(SIZE);
        free(ptr);
    }
    end = clock();

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("malloc time: %f seconds\n", cpu_time_used);
}

int main() {
    benchmark_vikalloc();
    benchmark_malloc();
    return 0;
}
