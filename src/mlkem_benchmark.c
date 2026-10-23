#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mlkem_native.h"

#define DEFAULT_ITERATIONS 1000UL
#define MAX_ITERATIONS 1000000UL
#define WARMUP_ITERATIONS 10UL

#ifndef MLKEM_BENCHMARK_IMPLEMENTATION
#define MLKEM_BENCHMARK_IMPLEMENTATION "portable-c"
#endif

static uint8_t g_public_key[MLKEM512_PUBLICKEYBYTES];
static uint8_t g_secret_key[MLKEM512_SECRETKEYBYTES];
static uint8_t g_ciphertext[MLKEM512_CIPHERTEXTBYTES];
static uint8_t g_shared_secret_sender[MLKEM_BYTES];
static uint8_t g_shared_secret_receiver[MLKEM_BYTES];
static volatile uint8_t g_sink;

static int monotonic_time_ns(uint64_t *value)
{
    struct timespec current;

    if (clock_gettime(CLOCK_MONOTONIC, &current) != 0) {
        return -1;
    }
    *value = (uint64_t)current.tv_sec * UINT64_C(1000000000) +
             (uint64_t)current.tv_nsec;
    return 0;
}

static int parse_iterations(int argc, char *argv[], unsigned long *iterations)
{
    char *end = NULL;
    unsigned long parsed;

    *iterations = DEFAULT_ITERATIONS;
    if (argc == 1) {
        return 0;
    }
    if (argc != 2) {
        return -1;
    }

    errno = 0;
    parsed = strtoul(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != '\0' || parsed == 0 ||
        parsed > MAX_ITERATIONS) {
        return -1;
    }
    *iterations = parsed;
    return 0;
}

static void print_result(const char *operation, unsigned long iterations,
                         uint64_t elapsed_ns)
{
    double total_ms = (double)elapsed_ns / 1000000.0;
    double mean_us = (double)elapsed_ns / ((double)iterations * 1000.0);
    double operations_per_second =
        (double)iterations * 1000000000.0 / (double)elapsed_ns;

    (void)printf("%s,ML-KEM-512,%s,%lu,%.3f,%.3f,%.2f\n",
                 MLKEM_BENCHMARK_IMPLEMENTATION, operation, iterations,
                 total_ms, mean_us, operations_per_second);
}

static int run_warmup(void)
{
    unsigned long index;

    for (index = 0; index < WARMUP_ITERATIONS; ++index) {
        if (mlkem_keypair(g_public_key, g_secret_key) != 0 ||
            mlkem_enc(g_ciphertext, g_shared_secret_sender, g_public_key) != 0 ||
            mlkem_dec(g_shared_secret_receiver, g_ciphertext, g_secret_key) != 0) {
            return -1;
        }
    }
    return memcmp(g_shared_secret_sender, g_shared_secret_receiver,
                  MLKEM_BYTES) == 0 ? 0 : -1;
}

int main(int argc, char *argv[])
{
    unsigned long iterations;
    unsigned long index;
    uint64_t start_ns;
    uint64_t end_ns;
    uint64_t keypair_ns;
    uint64_t encaps_ns;
    uint64_t decaps_ns;

    if (parse_iterations(argc, argv, &iterations) != 0) {
        (void)fprintf(stderr, "Usage: %s [iterations: 1..%lu]\n", argv[0],
                      MAX_ITERATIONS);
        return 2;
    }
    if (run_warmup() != 0) {
        (void)fprintf(stderr, "Warm-up or correctness check failed\n");
        return 1;
    }

    if (monotonic_time_ns(&start_ns) != 0) {
        return 1;
    }
    for (index = 0; index < iterations; ++index) {
        if (mlkem_keypair(g_public_key, g_secret_key) != 0) {
            (void)fprintf(stderr, "Key generation failed at iteration %lu\n",
                          index);
            return 1;
        }
        g_sink ^= g_public_key[0];
    }
    if (monotonic_time_ns(&end_ns) != 0) {
        return 1;
    }
    keypair_ns = end_ns - start_ns;

    if (monotonic_time_ns(&start_ns) != 0) {
        return 1;
    }
    for (index = 0; index < iterations; ++index) {
        if (mlkem_enc(g_ciphertext, g_shared_secret_sender, g_public_key) != 0) {
            (void)fprintf(stderr, "Encapsulation failed at iteration %lu\n",
                          index);
            return 1;
        }
        g_sink ^= g_ciphertext[0];
    }
    if (monotonic_time_ns(&end_ns) != 0) {
        return 1;
    }
    encaps_ns = end_ns - start_ns;

    if (monotonic_time_ns(&start_ns) != 0) {
        return 1;
    }
    for (index = 0; index < iterations; ++index) {
        if (mlkem_dec(g_shared_secret_receiver, g_ciphertext, g_secret_key) != 0) {
            (void)fprintf(stderr, "Decapsulation failed at iteration %lu\n",
                          index);
            return 1;
        }
        g_sink ^= g_shared_secret_receiver[0];
    }
    if (monotonic_time_ns(&end_ns) != 0) {
        return 1;
    }
    decaps_ns = end_ns - start_ns;

    if (memcmp(g_shared_secret_sender, g_shared_secret_receiver,
               MLKEM_BYTES) != 0) {
        (void)fprintf(stderr, "Final shared-secret check failed\n");
        return 1;
    }

    (void)printf("implementation,parameter_set,operation,iterations,total_ms,"
                 "mean_us,operations_per_second\n");
    print_result("keypair", iterations, keypair_ns);
    print_result("encaps", iterations, encaps_ns);
    print_result("decaps", iterations, decaps_ns);
    return 0;
}
