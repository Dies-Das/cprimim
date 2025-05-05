#include "image.h"
#include "color.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int index = y * image->columns * 3 + 3 * x;
    image->data[index] = color.r;
    image->data[index + 1] = color.g;
    image->data[index + 2] = color.b;
    return;
}
double cprimim_mse(const cprimim_Image *image1, const cprimim_Image *image2) {
    assert(image1->rows == image2->rows);
    assert(image1->columns == image2->columns);
    int N = image1->columns * image1->rows * 3;
    double result = 0;
    for (int k = 0; k < N; k++) {
        double diff = image1->data[k] - image2->data[k];
        result += (diff * diff);
    }
    result /= N;
    result = (result);
    return result;
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
