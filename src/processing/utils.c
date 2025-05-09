
#include "utils.h"

#include "assert.h"

#include <stdint.h>
#include <stdlib.h>

int cprimim_sign(int x) {
    if (x < 0) {
        return -1;
    }
    return 1;
}
int cprimim_max(int x, int y) {
    if (x > y) {
        return x;
    }
    return y;
}
// initialize to any nonzero seed:
static uint32_t rng_state = 2463534242u;

// returns next pseudorandom uint32 in [0,2^32)
static inline uint32_t fast_rand(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rng_state = x;
}
// returns uniform in [0…N), using a 32×32→64 multiply and a shift
static inline uint32_t fast_rand_range_mul(uint32_t N) {
    uint64_t prod = (uint64_t)fast_rand() * (uint64_t)N;
    return (uint32_t)(prod >> 32);
}

int cprimim_uniform_distribution(int lower, int upper) {
    int range = upper - lower;
    int result = fast_rand_range_mul(range);
    return result + lower;
}
int cprimim_clamp(int val, int lower, int upper) {
    return cprimim_max(0, -cprimim_max(-val, -upper));
}
