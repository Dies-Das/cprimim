#ifndef INTERNAL_H
#define INTERNAL_H
#include "color.h"
#include "image_internal.h"
#include "shapes.h"
#include "background.h"
#include <stddef.h>
#include <stdint.h>

typedef struct OptState{
    shape * shapes;
    uint64_t * grid_errors;
    uint64_t * cdf;
    size_t offset;
    Background background;

} OptState ;
typedef struct {
        ShapeType s;
        cprimim_BackgroundType bt;
        Image input;
        Image output;
        int rows;
        int columns;
        size_t nr_shapes;
        size_t initial_cells;
        size_t attempts;
        size_t background_shapes;

        OptState state;
} cprimim_Context;

cprimim_Context *cprimim_create_context(ShapeType s, cprimim_BackgroundType b, size_t nr_shapes,
                                        size_t initial_cells,
                                        size_t attempts,
                                       int columns, int rows, size_t background_shapes);
void cprimim_destroy_context(cprimim_Context *context);
void cprimim_set_input(cprimim_Context *context, uint8_t *buffer);
Image *cprimim_approximate(cprimim_Context *context);
#endif // !INTERNAL_H
