#include "bezier.h"
#include "triangle.h"
#include "cprimim_internal.h"
#include "image_internal.h"
#include "line.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

cprimim_Context *cprimim_create_context(ShapeType s, size_t nr_shapes,
                                       size_t candidates, size_t initial_shapes,
                                       size_t attempts,
                                       int columns, int rows) {
    cprimim_Context *ctx = malloc(sizeof *ctx);
    if (!ctx) return NULL;
    memset(ctx, 0, sizeof *ctx);
    size_t shape_size = 0;
    // utils_srand(time(NULL));

    ctx->rows = rows;
    ctx->columns = columns;
    ctx->candidates = candidates;
    ctx->initial_shapes = initial_shapes;
    ctx->nr_shapes = nr_shapes;
    ctx->attempts = attempts;
    ctx->s = s;
    ctx->state.shapes = malloc(sizeof(shape) * nr_shapes);
    ctx->state.grid_errors = malloc(sizeof(size_t)*initial_shapes);
    ctx->state.cdf = malloc(sizeof(size_t)*initial_shapes);
    ctx->output.data = malloc(columns * rows * 3);
    ctx->output.rows = rows;
    ctx->output.columns = columns;
    ctx->input.rows = rows;
    ctx->input.columns = columns;
    return ctx;
}
void cprimim_destroy_context(cprimim_Context *context) {
    free(context->output.data);
    free(context->state.shapes);
    free(context->state.cdf);
    free(context->state.grid_errors);
}
void cprimim_set_input(cprimim_Context *context, uint8_t *buffer) {
    context->input.data = buffer;
}
Image *cprimim_approximate(cprimim_Context *context) {
    Color avg = avg_color(&context->input);\
    context->state.background_color = avg;\
    set_background(&context->output, &avg);\
    switch (context->s) {
    case LINE:
        line_approx(context);
        break;
    case BEZIER:
        bezier_approx(context);
        break;
    case TRIANGLE:
        triangle_approx(context);
        break;
    default:
        break;
    }

    return &context->output;
}


uint8_t *cprimim_image_data(const Image *image){
    return image->data;
}
