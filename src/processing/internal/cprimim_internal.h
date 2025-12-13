
#ifndef CPRIMIM_INTERNAL_H
#define CPRIMIM_INTERNAL_H
#include "color.h"
#include "image_internal.h"
#include "shapes.h"
#include <stddef.h>
#include <stdint.h>
typedef struct cprimim_OptState{
    cprimim_shape * shapes;
    uint64_t * grid_errors;
    uint64_t * cdf;
    cprimim_Color background_color;
    cprimim_Profiler prof;

} cprimim_OptState ;
typedef struct {
        cprimim_ShapeType s;
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

cprimim_Context *cprimim_create_context(cprimim_ShapeType s, size_t nr_shapes,
                                       size_t candidates, size_t initial_shapes,
                                        size_t attempts,
                                       int columns, int rows);
void cprimim_destroy_context(cprimim_Context *context);
void cprimim_set_input(cprimim_Context *context, uint8_t *buffer);
cprimim_Image *cprimim_approximate(cprimim_Context *context);
#endif // !CPRIMIM_INTERNAL_H
