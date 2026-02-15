/* test_framework.c - Unit Test Framework */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;

int test_run(const char *name, int (*func)(void)) {
    total_tests++;
    printf("  Running: %s ... ", name);
    int result = func();
    if (result) { printf("PASS\n"); passed_tests++; }
    else { printf("FAIL\n"); failed_tests++; }
    return result;
}

void test_suite_print_summary(void) {
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", failed_tests);
    printf("Total: %d\n", total_tests);
}

int test_get_passed(void) { return passed_tests; }
int test_get_failed(void) { return failed_tests; }
int test_get_total(void) { return total_tests; }
void test_reset_counters(void) { total_tests = passed_tests = failed_tests = 0; }
