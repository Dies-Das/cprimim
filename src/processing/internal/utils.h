#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>
extern uint64_t rng_state;
void utils_srand(uint64_t seed);
static inline int sign(int x) {
    if (x < 0) {
        return -1;
    }
    return 1;
}
static inline int max(int x, int y) {
    if (x > y) {
        return x;
    }
    return y;
}

// returns next pseudorandom uint32 in [0,2^32)
static inline uint64_t fast_rand(void) {
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 2685821657736338717ULL;
}
// returns uniform in [0…N), using a 32×32→64 multiply and a shift
static inline uint32_t fast_rand_range_mul(uint32_t N) {
    uint32_t r = fast_rand();
    uint64_t prod = (uint64_t)r * (uint64_t)N;
    return (uint32_t)(prod >> 32);
}

static inline int uniform_distribution(int lower, int upper) {
    uint32_t range = (uint32_t)(upper - lower);
    int result = fast_rand_range_mul(range);
    return (int)(result + lower);
}

#define CLAMP(val, lower, upper) val = val<lower ? lower: val>upper? upper:val
static inline int clamp(int val, int lower, int upper) {
    if (val<lower) return lower;
    if (val>upper) return upper;
    return val;
}

static inline double u8_to_unit(uint8_t v)
{
    return (double)v / 255.0;
}
#endif // !UTILS_H
