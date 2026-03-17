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
        Comparator *restrict comparator = data;
    const int alpha = comparator->alpha;
    const int inv_alpha = 256 - alpha;
    
    Image *restrict output = comparator->other;
    size_t index = (y * output->columns + x) << 2;  // * 4 via shift
    
    // Load once
    int tgt_b = image->data[index];
    int tgt_g = image->data[index + 1];
    int tgt_r = image->data[index + 2];
    int cur_b = output->data[index];
    int cur_g = output->data[index + 1];
    int cur_r = output->data[index + 2];
    
    // Scaled diffs for optimal color computation
    int64_t db = (int64_t)tgt_b * 256 - inv_alpha * cur_b;
    int64_t dg = (int64_t)tgt_g * 256 - inv_alpha * cur_g;
    int64_t dr = (int64_t)tgt_r * 256 - inv_alpha * cur_r;
    
    comparator->sum_diffs[0] += db;
    comparator->sum_diffs[1] += dg;
    comparator->sum_diffs[2] += dr;
    comparator->sum_diffs_squared[0] += db * db;
    comparator->sum_diffs_squared[1] += dg * dg;
    comparator->sum_diffs_squared[2] += dr * dr;
    
    // Direct error (unscaled)
    int eb = cur_b - tgt_b;
    int eg = cur_g - tgt_g;
    int er = cur_r - tgt_r;
    comparator->error_old += eb*eb + eg*eg + er*er;
    
    comparator->counter++;
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
