#ifndef IMAGE_INTERNAL_H
#define IMAGE_INTERNAL_H
#include <stdint.h>
#include "utils.h"
#define CHANNELS 3
#define A 128
#include "color.h"
typedef struct cprimim_OptState cprimim_OptState;
typedef struct {
        unsigned char *data;
        int columns;
        int rows;
} cprimim_Image;
typedef struct {
        int64_t improvement;
        cprimim_Image *other;
        int64_t sum_diffs[3];
        int64_t sum_diffs_squared[3];
        int64_t error_old;
        int64_t max_error;
        int early_out;
        int counter;
} cprimim_Comparator;
typedef struct {
        cprimim_Image *output;
        cprimim_Color *color;
} cprimim_DrawData;
cprimim_Image cprimim_copy_image(const cprimim_Image *input);
void cprimim_set_image(const cprimim_Image *input, cprimim_Image *output);
// void cprimim_draw_pixel(cprimim_Image *image, int x, int y,
                        // cprimim_Color color);
// double cprimim_mse(const cprimim_Image *image1, const cprimim_Image *image2);
int sample_grid(cprimim_OptState* state, int regions);
void update_grid_errors(cprimim_OptState *state, const cprimim_Image *restrict  first, const cprimim_Image *restrict second, int columns, int rows, int regions);
cprimim_Color cprimim_avg_color(const cprimim_Image *image);
void cprimim_set_background(cprimim_Image *image, const cprimim_Color *color);
void cprimim_average_color_callback(cprimim_Image *image, int x, int y,
                                    void *data);
static inline void cprimim_draw_pixel_callback(cprimim_Image *restrict image, int x, int y,
                                 void *restrict data) {
    cprimim_DrawData *restrict drawdata = data;
    cprimim_Color *restrict color = drawdata->color;
    size_t index = CHANNELS * (y * image->columns + x);
    drawdata->output->data[index] =
        ((int)color->r + (int)drawdata->output->data[index]) / 2;
    drawdata->output->data[index + 1] =
        ((int)color->g + (int)drawdata->output->data[index + 1]) / 2;
    drawdata->output->data[index + 2] =
        ((int)color->b + (int)drawdata->output->data[index + 2]) / 2;
}
static inline void cprimim_compare_pixel_callback(cprimim_Image *restrict image, int x, int y,
                                    void *restrict data) {
    cprimim_Comparator *restrict comparator = data;
    cprimim_Image *restrict output = comparator->other;
    size_t index = CHANNELS * (y * output->columns + x);
    int old_r = image->data[index];
    int old_g = image->data[index + 1];
    int old_b = image->data[index + 2];
    int new_r = output->data[index];
    int new_g = output->data[index + 1];
    int new_b = output->data[index + 2];
    int64_t diff = -old_r * 255 + (255 - A) * new_r;
    comparator->sum_diffs[0] += diff;
    comparator->sum_diffs_squared[0] += diff * diff;
    diff = -old_g * 255 + (255 - A) * new_g;
    comparator->sum_diffs[1] += diff;
    comparator->sum_diffs_squared[1] += diff * diff;
    diff = -old_b * 255 + (255 - A) * new_b;
    comparator->sum_diffs[2] += diff;
    comparator->sum_diffs_squared[2] += diff * diff;
    comparator->counter++;
    for (int k = 0; k < 3; k++) {
        diff = output->data[index + k] - image->data[index + k];
        comparator->error_old += diff * diff;
    }
}
static inline uint64_t total_grid_error(uint64_t *grid_errors, size_t nr_initial){
    uint64_t result = 0;
    for(int k=0; k<nr_initial; k++){
        result += grid_errors[k];
    }
    return result;
}
#endif // !IMAGE_INTERNAL_H
