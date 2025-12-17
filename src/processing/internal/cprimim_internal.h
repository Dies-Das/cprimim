#ifndef INTERNAL_H
#define INTERNAL_H
#include "color.h"
#include "image_internal.h"
#include "shapes.h"
#include <stddef.h>
#include <stdint.h>


typedef struct OptState{
    shape * shapes;
    uint64_t * grid_errors;
    uint64_t * cdf;
    Color background_color;
    Profiler prof;

} OptState ;
typedef struct {
        ShapeType s;
        Image input;
        Image output;
        int rows;
        int columns;
        size_t nr_shapes;
        size_t candidates;
        size_t initial_shapes;
        size_t attempts;

        OptState state;
} cprimim_Context;

cprimim_Context *cprimim_create_context(ShapeType s, size_t nr_shapes,
                                       size_t candidates, size_t initial_shapes,
                                        size_t attempts,
                                       int columns, int rows);
void cprimim_destroy_context(cprimim_Context *context);
void cprimim_set_input(cprimim_Context *context, uint8_t *buffer);
Image *cprimim_approximate(cprimim_Context *context);
#endif // !INTERNAL_H
