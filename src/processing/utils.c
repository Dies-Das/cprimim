
// #include "utils.h"

#include <stdint.h>

uint64_t rng_state = 0x9E3779B97F4A7C15ULL;
void utils_srand(uint64_t seed) { rng_state = seed ? seed : 1; }
