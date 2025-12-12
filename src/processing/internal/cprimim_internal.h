
#ifndef CPRIMIM_INTERNAL_H
#define CPRIMIM_INTERNAL_H
#include "image_internal.h"
#include <stddef.h>
#include <stdint.h>
enum cprimim_shape { LINE, BEZIER, TRIANGLE, RECTANGLE, ELLIPSE };
typedef struct cprimim_OptState{
    void * shapes;
    void * candidate_shapes;
    uint64_t * grid_errors;
    uint64_t * cdf;
    cprimim_Profiler prof;

} cprimim_OptState ;
typedef struct {
        enum cprimim_shape s;
        cprimim_Image input;
        cprimim_Image output;
        int rows;
        int columns;
        size_t nr_shapes;
        size_t candidates;
        size_t initial_shapes;
        size_t attempts;

        cprimim_OptState state;
} cprimim_Context;

cprimim_Context *cprimim_create_context(enum cprimim_shape s, size_t nr_shapes,
                                       size_t candidates, size_t initial_shapes,
                                        size_t attempts,
                                       int columns, int rows);
void cprimim_destroy_context(cprimim_Context *context);
void cprimim_set_input(cprimim_Context *context, uint8_t *buffer);
cprimim_Image *cprimim_approximate(cprimim_Context *context);
#endif // !CPRIMIM_INTERNAL_H
