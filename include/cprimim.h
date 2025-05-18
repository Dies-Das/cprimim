#ifndef CPRIMIM_H
#define CPRIMIM_H
#include "image.h"
#include <stddef.h>
#include <stdint.h>
enum cprimim_shape { LINE, BEZIER, TRIANGLE, RECTANGLE, ELLIPSE };
typedef struct {
        enum cprimim_shape s;
        cprimim_IndexBuffer *result_buffer;
        cprimim_IndexBuffer *working_buffer;
        void *shapes;
        void *candidate_shapes;
        cprimim_Image input;
        cprimim_Image output;
        int rows;
        int columns;
        size_t nr_shapes;
        size_t candidates;
        size_t threads;
        size_t attempts;
} cprimim_Context;

cprimim_Context cprimim_create_context(enum cprimim_shape s, size_t nr_shapes,
                                       size_t candidates, size_t threads,
                                       size_t attempts, int columns, int rows);
void cprimim_destroy_context(cprimim_Context *context);
void cprimim_set_input(cprimim_Context *context, uint8_t *buffer);
cprimim_Image *cprimim_approximate(cprimim_Context *context);
#endif // !CPRIMIM_H
