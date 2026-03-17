#ifndef CPRIMIM_H
#define CPRIMIM_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
enum cprimim_shape { LINE, BEZIER, TRIANGLE, RECTANGLE, ELLIPSE };
typedef enum
{
    NONE,
    UNIFORM_TRIANGULATION,
    DELAUNAY
} cprimim_BackgroundType;
typedef struct cprimim_Context cprimim_Context;
typedef struct cprimim_Image cprimim_Image;

cprimim_Context *cprimim_create_context(enum cprimim_shape s, cprimim_BackgroundType b, size_t nr_shapes,
                                        size_t initial_cells, size_t attempts, int columns,
                                        int rows, size_t background_shapes, uint8_t alpha);
void cprimim_destroy_context(cprimim_Context *context);
void cprimim_reset_context(cprimim_Context *context);
void cprimim_set_input(cprimim_Context *context, uint8_t *buffer);
cprimim_Image *cprimim_approximate(cprimim_Context *context);
uint8_t *cprimim_image_data(const cprimim_Image *);
int cprimim_to_svg(cprimim_Context *context, FILE * file);
#endif // !CPRIMIM_H
