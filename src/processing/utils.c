
#include "utils.h"

#include "assert.h"

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

int cprimim_uniform_distribution(int lower, int upper) {
    int range = upper - lower;

    int copies =

        RAND_MAX / range; // we can fit n-copies of [0...range-1] into RAND_MAX

    // Use rejection sampling to avoid distribution errors

    int limit = range * copies;
    int myRand = -1;
    while (myRand < 0 || myRand >= limit) {
        myRand = rand();
    }

    return myRand / copies + lower; // note that this involves the high-bits
}
int cprimim_clamp(int val, int lower, int upper) {
    return cprimim_max(0, -cprimim_max(-val, -upper));
}
