#ifndef IMAGE_INTERNAL_H
#define IMAGE_INTERNAL_H
#include "utils.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#define CHANNELS 4
#define CHANNELS_COLOR 3
#include "color.h"
typedef struct OptState OptState;
typedef struct
{
    unsigned char *data;
    int columns;
    int rows;
} Image;
typedef struct
{
    int64_t improvement;
    Image *other;
    int64_t sum_diffs[3];
    int64_t sum_diffs_squared[3];
    int64_t error_old;
    int64_t max_error;
    int early_out;
    int counter;
    int iter;
    int stride;
    uint8_t alpha;
} Comparator;
typedef struct
{
    Image *output;
    Color *color;
    uint8_t alpha;
} DrawData;
Image cprimim_copy_image(const Image *input);
void cprimim_set_image(const Image *input, Image *output);
// void draw_pixel(Image *image, int x, int y,
// Color color);
// double mse(const Image *image1, const Image *image2);
int cprimim_sample_grid(OptState *state, int regions);
void cprimim_update_grid_errors(OptState *state, const Image *restrict first,
                                const Image *restrict second, int columns, int rows, int regions);
Color cprimim_avg_color(const Image *image);
void cprimim_set_background(Image *image, const Color *color);
void cprimim_average_color_callback(Image *image, int x, int y, void *data);
static inline void draw_pixel_callback(Image *restrict image, int x, int y, void *restrict data)
{
    assert(x >= 0 && y >= 0 && x < image->columns && y < image->rows);
    DrawData *restrict drawdata = data;
    Color *restrict color = drawdata->color;
    uint8_t alpha = color->alpha;
    size_t index = CHANNELS * (y * image->columns + x);
    drawdata->output->data[index] =
        ((int)color->b * alpha + (int)drawdata->output->data[index] * (256 - alpha)) >> 8;
    drawdata->output->data[index + 1] =
        ((int)color->g * alpha + (int)drawdata->output->data[index + 1] * (256 - alpha)) >> 8;
    drawdata->output->data[index + 2] =
        ((int)color->r * alpha + (int)drawdata->output->data[index + 2] * (256 - alpha)) >> 8;
}
static inline void compare_pixel_callback(Image *restrict image, int x, int y, void *restrict data)
{
    assert(x >= 0 && y >= 0 && x < image->columns && y < image->rows);
    Comparator *restrict comparator = data;
    uint8_t alpha = comparator->alpha;

    comparator->iter++;
    if (comparator->iter % comparator->stride != 0)
    {
        return;
    }
    Image *restrict output = comparator->other;
    size_t index = CHANNELS * (y * output->columns + x);
    int old_b = image->data[index];
    int old_g = image->data[index + 1];
    int old_r = image->data[index + 2];
    int new_b = output->data[index];
    int new_g = output->data[index + 1];
    int new_r = output->data[index + 2];
    int64_t diff = (int64_t)old_b * 256 - (256 - alpha) * new_b;
    comparator->sum_diffs[0] += diff;
    comparator->sum_diffs_squared[0] += diff * diff;
    diff = (int64_t)old_g * 256 - (256 - alpha) * new_g;
    comparator->sum_diffs[1] += diff;
    comparator->sum_diffs_squared[1] += diff * diff;
    diff = (int64_t)old_r * 256 - (256 - alpha) * new_r;
    comparator->sum_diffs[2] += diff;
    comparator->sum_diffs_squared[2] += diff * diff;
    comparator->counter++;
    diff = new_r - old_r;
    comparator->error_old += 1 * diff * diff;
    diff = new_g - old_g;
    comparator->error_old += 1 * diff * diff;
    diff = new_b - old_b;
    comparator->error_old += diff * diff;
}
static inline uint64_t total_grid_error(uint64_t *grid_errors, size_t nr_initial)
{
    uint64_t result = 0;
    for (size_t k = 0; k < nr_initial; k++)
    {
        result += grid_errors[k];
    }
    return result;
}

// int to_svg(Context *context, FILE * file);
#endif // !IMAGE_INTERNAL_H
