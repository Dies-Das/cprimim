#include "background.h"
#include "bezier.h"
#include "cprimim_internal.h"
#include "delaunay.h"
#include "image_internal.h"
#include "line.h"
#include "sample.h"
#include "triangle.h"
#include "triangulation.h"
#include "utils.h"
#include <cairo/cairo.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void initialize_cairo_context(cprimim_Context *context);
cprimim_Context *cprimim_create_context(ShapeType s, cprimim_BackgroundType b, size_t nr_shapes,
                                        size_t initial_cells, size_t attempts, int columns,
                                        int rows, size_t background_shapes, uint8_t alpha)
{
    cprimim_Context *ctx = malloc(sizeof *ctx);
    if (!ctx)
        return NULL;
    memset(ctx, 0, sizeof *ctx);

    utils_srand(time(NULL) * 0x9E3779B97F4A7C15ULL);
    ctx->bt = b;
    ctx->rows = rows;
    ctx->columns = columns;
    ctx->initial_cells = initial_cells;
    ctx->nr_shapes = nr_shapes;
    ctx->attempts = attempts;
    ctx->background_shapes = background_shapes;
    ctx->s = s;
    ctx->alpha = alpha;
    ctx->state.shapes = malloc(sizeof(shape) * nr_shapes);
    ctx->state.grid_errors = malloc(sizeof(size_t) * initial_cells);
    ctx->state.cdf = malloc(sizeof(size_t) * initial_cells);
    if (ctx->bt == UNIFORM_TRIANGULATION)
    {
        ctx->state.background.triag.triangles = malloc(sizeof(triangle) * background_shapes);
    }
    if (ctx->bt == DELAUNAY)
    {
        ctx->state.background.triag.triangles = malloc(sizeof(triangle) * background_shapes * 5);
    }
    ctx->output.data = malloc(columns * rows * CHANNELS);
    ctx->output.rows = rows;
    ctx->output.columns = columns;
    ctx->input.rows = rows;
    ctx->input.columns = columns;
    initialize_cairo_context(ctx);
    return ctx;
}
void cprimim_destroy_context(cprimim_Context *context)
{
    free(context->output.data);
    free(context->state.shapes);
    free(context->state.cdf);
    free(context->state.grid_errors);
    switch (context->bt) {
        case UNIFORM_TRIANGULATION:
        case DELAUNAY:
        free(context->state.background.triag.triangles);
        default:
        break;
    }
    cairo_destroy(context->cairo.cr);
    cairo_surface_destroy(context->cairo.surf);
    free(context);
}
void cprimim_reset_context(cprimim_Context *context){

    memset(context->output.data, 0, CHANNELS*context->output.rows*context->output.columns);
    context->state.background.triag.size = 0;
}
void cprimim_set_input(cprimim_Context *context, uint8_t *buffer)
{
    context->input.data = buffer;
}
Image *cprimim_approximate(cprimim_Context *context)
{
    Color avg = cprimim_avg_color(&context->input);
    context->state.background.average = avg;
    cprimim_set_background(&context->output, &avg);
    switch (context->bt)
    {
    case NONE:
    {
    }
    break;
    case UNIFORM_TRIANGULATION:
        cprimim_set_triangulation(context);
        break;
    case DELAUNAY:
        cprimim_delaunay_triangulation(context);
        break;
    default:
        break;
    }
    switch (context->s)
    {
    case LINE:
        cprimim_line_approx(context);
        break;
    case BEZIER:
        cprimim_bezier_approx(context);
        break;
    case TRIANGLE:
        cprimim_triangle_approx(context);
        break;
    default:
        break;
    }

    return &context->output;
}

uint8_t *cprimim_image_data(const Image *image)
{
    return image->data;
}
void initialize_cairo_context(cprimim_Context *context)
{
    Image *img = &context->output;
    CairoContext* cairo = &context->cairo;

    cairo->data = img->data;
    cairo->stride = img->columns * 4;

    cairo->surf = cairo_image_surface_create_for_data(
        cairo->data, CAIRO_FORMAT_RGB24, img->columns, img->rows, cairo->stride);

    cairo->cr = cairo_create(cairo->surf);

    cairo_set_antialias(cairo->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_join(cairo->cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_cap(cairo->cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_operator(cairo->cr, CAIRO_OPERATOR_OVER);
}
