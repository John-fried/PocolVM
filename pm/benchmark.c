/* benchmark.c - Performance Benchmark Suite */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define gettickcount GetTickCount
#else
#include <sys/time.h>
#endif

typedef struct {
    const char *name;
    double time_ms;
} BenchmarkResult;

static BenchmarkResult results[32];
static int result_count = 0;

static double get_time_ms(void) {
#ifdef _WIN32
    return (double)GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#endif
}

void benchmark_run(const char *name, void (*func)(void), int iterations) {
    printf("Running: %s (%d iterations)\n", name, iterations);
    double start = get_time_ms();
    for (int i = 0; i < iterations; i++) func();
    double elapsed = get_time_ms() - start;
    printf("  Time: %.2f ms\n", elapsed);
    if (result_count < 32) {
        results[result_count].name = name;
        results[result_count].time_ms = elapsed;
        result_count++;
    }
}

void benchmark_summary(void) {
    printf("\n=== Summary ===\n");
    for (int i = 0; i < result_count; i++) {
        printf("%s: %.2f ms\n", results[i].name, results[i].time_ms);
    }
}

void bench_empty(void) { }

int main(int argc, char **argv) {
    printf("PocolVM Benchmark Suite\n");
    printf("========================\n");
    benchmark_run("Empty", bench_empty, 1000000);
    benchmark_summary();
    return 0;
}