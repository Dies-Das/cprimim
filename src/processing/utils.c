
#include "utils.h"

#include <stdint.h>

uint32_t rng_state = 2463534242u;

void utils_srand(uint64_t seed) { rng_state = seed ? seed : 1; }
