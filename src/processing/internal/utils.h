#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#define MUTATION_DISTANCE 40
#define ALPHA 128
#define A 128
extern _Thread_local uint64_t rng_state;
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

static inline int cprimim_uniform_distribution(int lower, int upper) {
    uint32_t range = (uint32_t)(upper - lower);
    int result = fast_rand_range_mul(range);
    return (int)(result + lower);
}
static inline int cprimim_clamp(int val, int lower, int upper) {
    if (val<lower) return lower;
    if (val>upper) return upper;
    return val;
}
static inline uint64_t cprimim_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

typedef struct {
        
    uint64_t approx_ns;
    uint64_t grid_ns;
    uint64_t bestfit_ns;
    uint64_t draw_ns;

    
    uint64_t shapes_done;

    
    uint64_t mutations_total;       
    uint64_t mutations_accepted;    
    uint64_t mutations_rejected;    
    uint64_t bestfit_calls;         

    
    uint64_t restarts;              
    uint64_t stop_by_max_tries;     
    uint64_t stop_by_stagnation;    

    
    int64_t  sum_start_improvement; 
    int64_t  sum_best_improvement;  
    int64_t  sum_delta_improvement; 

    
    uint64_t mut_hist[64]; 
} cprimim_Profiler;
#define TBEGIN(var) uint64_t var = cprimim_now_ns()
#define TACCUM(dst, var) do { (dst) += (cprimim_now_ns() - (var)); } while (0)
static void cprimim_print_profile(const cprimim_Profiler *p)
{
    double total_ms  = p->approx_ns  / 1e6;
    double grid_ms   = p->grid_ns    / 1e6;
    double best_ms   = p->bestfit_ns / 1e6;
    double draw_ms   = p->draw_ns    / 1e6;

    uint64_t shapes = p->shapes_done;
    uint64_t muts   = p->mutations_total;
    uint64_t acc    = p->mutations_accepted;
    uint64_t rej    = p->mutations_rejected;
    uint64_t fits   = p->bestfit_calls;

    double acc_rate = muts ? (double)acc / (double)muts : 0.0;
    double muts_per_shape = shapes ? (double)muts / (double)shapes : 0.0;
    double fits_per_shape = shapes ? (double)fits / (double)shapes : 0.0;
    double muts_per_acc   = acc ? (double)muts / (double)acc : 0.0;

    double avg_start = shapes ? (double)p->sum_start_improvement / (double)shapes : 0.0;
    double avg_best  = shapes ? (double)p->sum_best_improvement  / (double)shapes : 0.0;
    double avg_delta = shapes ? (double)p->sum_delta_improvement / (double)shapes : 0.0;

    fprintf(stderr, "==== cprimim profile ====\n");

    fprintf(stderr, "time:\n");
    fprintf(stderr, "  approx:   %.3f ms\n", total_ms);
    fprintf(stderr, "  grid:     %.3f ms (%.1f%%)\n", grid_ms, 100.0 * grid_ms / total_ms);
    fprintf(stderr, "  best_fit: %.3f ms (%.1f%%)\n", best_ms, 100.0 * best_ms / total_ms);
    fprintf(stderr, "  draw:     %.3f ms (%.1f%%)\n", draw_ms, 100.0 * draw_ms / total_ms);

    fprintf(stderr, "\nwork:\n");
    fprintf(stderr, "  shapes done:        %llu\n", (unsigned long long)shapes);
    fprintf(stderr, "  mutations total:    %llu\n", (unsigned long long)muts);
    fprintf(stderr, "  best_fit calls:     %llu\n", (unsigned long long)fits);

    fprintf(stderr, "\nmutation behavior:\n");
    fprintf(stderr, "  accepted:           %llu\n", (unsigned long long)acc);
    fprintf(stderr, "  rejected:           %llu\n", (unsigned long long)rej);
    fprintf(stderr, "  acceptance rate:    %.2f%%\n", 100.0 * acc_rate);
    fprintf(stderr, "  muts / shape:       %.2f\n", muts_per_shape);
    fprintf(stderr, "  bestfits / shape:   %.2f\n", fits_per_shape);
    fprintf(stderr, "  muts / accepted:    %.2f\n", muts_per_acc);

    // fprintf(stderr, "\nstop reasons:\n");
    // fprintf(stderr, "  stop by max tries:  %llu\n",
    //         (unsigned long long)p->stop_by_max_tries);
    // fprintf(stderr, "  stop by stagnation: %llu\n",
    //         (unsigned long long)p->stop_by_stagnation);
    // fprintf(stderr, "  restarts:           %llu\n",
    //         (unsigned long long)p->restarts);

    fprintf(stderr, "\nquality (avg per shape):\n");
    fprintf(stderr, "  start improvement:  %.3e\n", avg_start);
    fprintf(stderr, "  best improvement:   %.3e\n", avg_best);
    fprintf(stderr, "  delta improvement:  %.3e\n", avg_delta);

    fprintf(stderr, "\nmutation histogram (log2 buckets):\n");
    fprintf(stderr, "  bucket : count\n");
    for (int i = 0; i < 64; ++i) {
        if (p->mut_hist[i]) {
            uint64_t lo = (i == 0) ? 0 : (1ULL << (i - 1));
            uint64_t hi = (1ULL << i);
            fprintf(stderr, "  [%5llu .. %5llu): %llu\n",
                    (unsigned long long)lo,
                    (unsigned long long)hi,
                    (unsigned long long)p->mut_hist[i]);
        }
    }

    fprintf(stderr, "========================\n");
}
// static void cprimim_print_profile(const cprimim_Profiler *p) {
//     double total_ms  = p->approx_ns  / 1e6;
//     double grid_ms   = p->grid_ns    / 1e6;
//     double best_ms   = p->bestfit_ns / 1e6;
//     double draw_ms   = p->draw_ns    / 1e6;
//
//     fprintf(stderr, "approx:   %.3f ms\n", total_ms);
//     fprintf(stderr, "grid:     %.3f ms (%.1f%%)\n", grid_ms, 100.0*grid_ms/total_ms);
//     fprintf(stderr, "best_fit: %.3f ms (%.1f%%)\n", best_ms, 100.0*best_ms/total_ms);
//     fprintf(stderr, "draw:     %.3f ms (%.1f%%)\n", draw_ms, 100.0*draw_ms/total_ms);
//     fprintf(stderr, "mutations: %llu\n", (unsigned long long)p->mutations_total);
//     fprintf(stderr, "shapes:    %llu\n", (unsigned long long)p->shapes_done);
// }
static inline int bucket_u64(uint64_t x) {
    int b = 0;
    while (x > 1 && b < 63) { x >>= 1; b++; }
    return b;
}
#endif // !UTILS_H
