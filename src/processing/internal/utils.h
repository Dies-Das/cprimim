#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#define MUTATION_DISTANCE 40
extern uint32_t rng_state;
void utils_srand(uint64_t seed);
static inline int cprimim_sign(int x) {
    if (x < 0) {
        return -1;
    }
    return 1;
}
static inline int cprimim_max(int x, int y) {
    if (x > y) {
        return x;
    }
    return y;
}
// initialize to any nonzero seed:

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

static inline int cprimim_uniform_distribution(int lower, int upper) {
    int range = upper - lower;
    int result = fast_rand_range_mul(range);
    return result + lower;
}
static inline int cprimim_clamp(int val, int lower, int upper) {
    return cprimim_max(0, -cprimim_max(-val, -upper));
}
#endif // !UTILS_H
