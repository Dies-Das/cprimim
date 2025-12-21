#include "bezier.h"
#include "color.h"
#include "cprimim_internal.h"
#include "image_internal.h"
#include "line.h"
#include "shapes.h"
#include "triangle.h"
#include "utils.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint64_t rectangle_error(const Image *restrict first, const Image *restrict second, int x1,
                                int x2, int y1, int y2);
Image cprimim_copy_image(const Image *input)
{
    Image output = {0};
    long size = input->rows * input->columns * CHANNELS;
    output.data = malloc(size);
    if (output.data == NULL)
    {
        printf("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    output.rows = input->rows;
    output.columns = input->columns;
    memcpy(output.data, input->data, size);
    return output;
}
#define ALPHA 128
// void draw_pixel(Image *image, int x, int y, Color color)
// {
//     int index = y * image->columns * CHANNELS + CHANNELS * x;
//     unsigned dr = image->data[index], dg = image->data[index + 1], db = image->data[index + 2];
//     image->data[index] = (color.r * ALPHA + dr * (255 - ALPHA)) / 255;
//     image->data[index + 1] = (color.g * ALPHA + dg * (255 - ALPHA)) / 255;
//     image->data[index + 2] = (color.b * ALPHA + db * (255 - ALPHA)) / 255;
//     return;
// }
double cprimim_mse(const Image *restrict image1, const Image *restrict image2)
{
    assert(image1->rows == image2->rows);
    assert(image1->columns == image2->columns);
    int N = image1->columns * image1->rows * 3;
    uint64_t result = 0;
    for (int k = 0; k < N; k++)
    {
        int diff = (int)image1->data[k] - (int)image2->data[k];
        result += (uint64_t)(diff * diff);
    }
    return (double)result / (double)N;
}
Color cprimim_avg_color(const Image *image)
{
    double sum[] = {0, 0, 0};
    size_t n = image->rows * image->columns * 3;
    for (size_t k = 0; k < n; k++)
    {
        sum[k % 3] += image->data[k];
    }
    for (int k = 0; k < 3; k++)
    {
        sum[k] /= (double)n / 3;
        sum[k] += 0.5;
        sum[k] = floor(sum[k]);
    }
    return (Color){sum[0], sum[1], sum[2]};
}
void cprimim_set_background(Image *image, const Color *color)
{
    size_t n = image->rows * image->columns * 3;
    for (size_t k = 0; k < n; k += 3)
    {
        image->data[k] = color->r;
        image->data[k + 1] = color->g;
        image->data[k + 2] = color->b;
    }
}
void cprimim_set_image(const Image *input, Image *output)
{
    assert(input->columns == output->columns && input->rows == output->rows);
    memcpy(output->data, input->data, input->rows * input->columns * 3);
}
// void average_color_callback(Image *restrict image, int x, int y,
//                                     void *restrict data) {
//
//     assert(x >= 0 && y >= 0 && x < image->columns && y < image->rows);
//     AvgColor *final_color = data;
//     size_t index = y * image->columns * CHANNELS + x * CHANNELS;
//     final_color->r += image->data[index];
//     final_color->g += image->data[index + 1];
//     final_color->b += image->data[index + 2];
//     final_color->count++;
// }
void cprimim_update_grid_errors(OptState *state, const Image *restrict first,
                                const Image *restrict second, int columns, int rows, int regions)
{
    int Gx = (int)floor(sqrt((double)regions * (double)columns / (double)rows));
    if (Gx < 1)
        Gx = 1;
    int Gy = (regions + Gx - 1) / Gx;
    if (Gy < 1)
        Gy = 1;
    int cell_w = columns / Gx;
    int cell_h = rows / Gy;
    if (cell_w < 1)
        cell_w = 1;
    if (cell_h < 1)
        cell_h = 1;
    for (int region = 0; region < regions; region++)
    {
        int cell_x = region % Gx;
        int cell_y = region / Gx;
        int x1 = cell_x * cell_w;
        int y1 = cell_y * cell_h;

        int x2 = (cell_x == Gx - 1) ? columns : (x1 + cell_w);
        int y2 = (cell_y == Gy - 1) ? rows : (y1 + cell_h);
        state->grid_errors[region] = rectangle_error(first, second, x1, x2, y1, y2);
    }
    state->cdf[0] = state->grid_errors[0];
    for (int region = 1; region < regions; region++)
    {
        state->cdf[region] = state->grid_errors[region] + state->cdf[region - 1];
    }
}
int cprimim_sample_grid(OptState *state, int regions)
{
    uint64_t total = state->cdf[regions - 1];
    if (total == 0)
    {
    }
    uint64_t sample = fast_rand();
    sample %= state->cdf[regions - 1];
    for (int k = 0; k < regions; k++)
    {
        if (sample < state->cdf[k])
        {
            return k;
        }
    }
    return regions - 1;
}
static uint64_t rectangle_error(const Image *restrict first, const Image *restrict second, int x1,
                                int x2, int y1, int y2)
{
    const int columns = first->columns;
    size_t index = 0;
    uint64_t error = 0;
    uint64_t local_error = 0;
    if (x1 < 0)
        x1 = 0;
    if (y1 < 0)
        y1 = 0;
    if (x2 > columns)
        x2 = columns;
    if (y2 > first->rows)
        y2 = first->rows;
    for (int y = y1; y < y2; y++)
    {
        size_t row = (size_t)y * (size_t)columns * CHANNELS;
        for (int x = x1; x < x2; x++)
        {

            index = row + x * CHANNELS;
            for (int k = 0; k < CHANNELS; k++)
            {
                local_error = first->data[index + k] - second->data[index + k];
                local_error *= local_error;
                error += local_error;
            }
        }
    }
    return error;
}

static void print_svg_header(FILE *file, int width, int height)
{
    fprintf(file,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\"\
            xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n\
            version=\"1.1\" baseProfile=\"full\"\n\
            viewBox=\"0 0 %i %i\">\n\
            <title>Approximated image</title>\n\
            <desc>File createt using cprimim.</desc>\n",
            width, height);
}
static void print_svg_background_rectangle(FILE *file, int width, int height, Color color)
{
    fprintf(file, "<rect x=\"0\" y=\"0\" width=\"%i\" height=\"%i\" fill=\"rgb(%u,%u,%u)\" />",
            width, height, color.r, color.g, color.b);
}
int cprimim_to_svg(cprimim_Context *context, FILE *file)
{
    Background *background = &context->state.background;
    print_svg_header(file, context->columns, context->rows);
    print_svg_background_rectangle(file, context->columns, context->rows, background->average);
    switch (context->bt)
    {
    case NONE:

        break;
    case UNIFORM_TRIANGULATION:
    case DELAUNAY:
        for (size_t k = 0; k < background->triag.size; k++)
        {
            cprimim_write_triangle_svg(file, &background->triag.triangles[k], 1.0);
        }

        break;

    default:
        break;
    }
    shape *shapes = context->state.shapes;
    double alpha = ((double)context->alpha)/255;
    for (size_t k = 0; k < context->nr_shapes; k++)
    {
        switch (shapes[k].s)
        {
        case BEZIER:
            cprimim_write_bezier_svg(file, &shapes[k].shape.bezier, alpha);
            break;
        case LINE:
            cprimim_write_line_svg(file, &shapes[k].shape.line, alpha);
            break;
        case TRIANGLE:
            cprimim_write_triangle_svg(file, &shapes[k].shape.triangle, alpha);
            break;
        default:
            break;
        }
    }

    fprintf(file, "</svg>");
    return 0;
}
