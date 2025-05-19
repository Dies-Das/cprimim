#include "bezier.h"
#include "cprimim.h"
#include "image.h"
#include "line.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

cprimim_Context cprimim_create_context(enum cprimim_shape s, size_t nr_shapes,
                                       size_t candidates, size_t threads,
                                       size_t attempts, int columns, int rows) {
    cprimim_Context result = {0};
    size_t shape_size = 0;
    switch (s) {
    case LINE:
        shape_size = sizeof(cprimim_Line);
        break;
    case BEZIER:
        shape_size = sizeof(cprimim_Bezier);
        break;
    case TRIANGLE:
        break;
    case RECTANGLE:
        break;
    case ELLIPSE:
        break;
    default:
        fprintf(stderr, "Shape either does not exist or is not implemented.\n");
        return result;
    }
    utils_srand(time(NULL));

    result.rows = rows;
    result.columns = columns;
    result.candidates = candidates;
    result.threads = threads;
    result.nr_shapes = nr_shapes;
    result.attempts = attempts;
    result.s = s;
    result.shapes = malloc(shape_size * nr_shapes);
    result.result_buffer.indices =
        malloc(sizeof(size_t) * (columns + rows) * candidates);
    result.working_buffer.indices =
        malloc(sizeof(size_t) * (columns + rows) * candidates);
    result.output.data = malloc(columns * rows * 3);
    result.output.rows = rows;
    result.output.columns = columns;
    result.input.rows = rows;
    result.input.columns = columns;
    return result;
}
void cprimim_destroy_context(cprimim_Context *context) {
    free(context->result_buffer.indices);
    free(context->working_buffer.indices);
    free(context->output.data);
    free(context->shapes);
}
void cprimim_set_input(cprimim_Context *context, uint8_t *buffer) {
    context->input.data = buffer;
}
cprimim_Image *cprimim_approximate(cprimim_Context *context) {
    switch (context->s) {
    case LINE:
        cprimim_line_approx(&context->input, &context->output,
                            context->nr_shapes, context->candidates, 4);
        break;
    case BEZIER:
        cprimim_bezier_approx(context);
        break;
    default:
        break;
    }

    return &context->output;
}
