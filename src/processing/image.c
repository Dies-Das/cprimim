#include "image.h"
#include "color.h"
#include "image_internal.h"
#include <assert.h>
#include <math.h>
#include <omp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ALPHA 122
cprimim_Image cprimim_copy_image(const cprimim_Image *input) {
    cprimim_Image output = {0};
    long size = input->rows * input->columns * 3;
    output.data = malloc(size);
    if (output.data == NULL) {
        printf("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    output.rows = input->rows;
    output.columns = input->columns;
    memcpy(output.data, input->data, size);
    return output;
}
void cprimim_draw_pixel(cprimim_Image *image, int x, int y,
                        cprimim_Color color) {
    int index = y * image->columns * CHANNELS + CHANNELS * x;
    unsigned dr = image->data[index], dg = image->data[index + 1],
             db = image->data[index + 2];
    image->data[index] = (color.r * ALPHA + dr * (255 - ALPHA)) / 255;
    image->data[index + 1] = (color.g * ALPHA + dg * (255 - ALPHA)) / 255;
    image->data[index + 2] = (color.b * ALPHA + db * (255 - ALPHA)) / 255;
    return;
}
double cprimim_mse(const cprimim_Image *restrict image1,
                   const cprimim_Image *restrict image2) {
    assert(image1->rows == image2->rows);
    assert(image1->columns == image2->columns);
    int N = image1->columns * image1->rows * 3;
    uint64_t result = 0;
#pragma omp simd reduction(+ : result)
    for (int k = 0; k < N; k++) {
        int diff = (int)image1->data[k] - (int)image2->data[k];
        result += (uint64_t)(diff * diff);
    }
    return (double)result / (double)N;
}
cprimim_Color cprimim_avg_color(const cprimim_Image *image) {
    double sum[] = {0, 0, 0};
    size_t n = image->rows * image->columns * 3;
    for (size_t k = 0; k < n; k++) {
        sum[k % 3] += image->data[k];
    }
    for (int k = 0; k < 3; k++) {
        sum[k] /= (double)n / 3;
        sum[k] += 0.5;
        sum[k] = floor(sum[k]);
    }
    return (cprimim_Color){sum[0], sum[1], sum[2]};
}
void cprimim_set_background(cprimim_Image *image, const cprimim_Color *color) {
    size_t n = image->rows * image->columns * 3;
    for (size_t k = 0; k < n; k += 3) {
        image->data[k] = color->r;
        image->data[k + 1] = color->g;
        image->data[k + 2] = color->b;
    }
}
void cprimim_set_image(const cprimim_Image *input, cprimim_Image *output) {
    assert(input->columns == output->columns && input->rows == output->rows);
    memcpy(output->data, input->data, input->rows * input->columns * 3);
}
void cprimim_draw_pixel_callback(cprimim_Image *image, uint64_t index,
                                 void *data) {
    assert(index >= 0 && index + 2 < image->rows * image->columns * 3);
    cprimim_Color *color = data;

    image->data[index] = ((int)color->r + (int)image->data[index]) / 2;
    image->data[index + 1] = ((int)color->g + (int)image->data[index + 1]) / 2;
    image->data[index + 2] = ((int)color->b + (int)image->data[index + 2]) / 2;
}
void cprimim_average_color_callback(cprimim_Image *image, int x, int y,
                                    void *data) {

    assert(x >= 0 && y >= 0 && x < image->columns && y < image->rows);
    cprimim_AvgColor *final_color = data;
    size_t index = y * image->columns * CHANNELS + x * CHANNELS;
    final_color->r += image->data[index];
    final_color->g += image->data[index + 1];
    final_color->b += image->data[index + 2];
    final_color->count++;
}
void cprimim_compare_pixel_callback(cprimim_Image *image, cprimim_Image *output,
                                    uint64_t index, void *data) {
    assert(index >= 0 && index + 2 < image->rows * image->columns * 3);
    cprimim_Comparator *comparator = data;
    int old_mse = 0;
    int new_mse = 0;
    int diff = 0;
    for (int k = 0; k < 3; k++) {
        diff = (int)image->data[index + k] -
               (int)comparator->other->data[index + k];
        old_mse += diff * diff;
    }

    diff = (int)image->data[index] -
           ((int)comparator->color->r + (int)output->data[index]) / 2;
    new_mse += diff * diff;
    diff = (int)image->data[index + 1] -
           ((int)comparator->color->g + (int)output->data[index + 1]) / 2;
    new_mse += diff * diff;
    diff = (int)image->data[index + 2] -
           ((int)comparator->color->b + (int)output->data[index + 2]) / 2;
    new_mse += diff * diff;
    comparator->improvement = (old_mse - new_mse);
}
