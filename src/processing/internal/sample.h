#ifndef SAMPLE_H
#define SAMPLE_H

#include "background.h"
#include "cprimim_internal.h"
#include "image_internal.h"
#include "point.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
const static int sobel_x[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
const static int sobel_y[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};

static inline size_t binary_search(uint64_t *cdf, uint64_t value, size_t n)
{
    size_t lo = 0, hi = n - 1;
    while (lo < hi)
    {
        size_t mid = (lo + hi) / 2;
        if (cdf[mid] <= value)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    return lo;
}
static inline int pixel_gray(Image *image, int x, int y)
{
    size_t idx = CHANNELS * (y * image->columns + x);
    int result = 0;
    for (int k = 0; k < CHANNELS; k++)
    {
        result += image->data[idx + k];
    }
    return result / 3;
}
static inline void compute_sobel(Image *image, uint64_t *gradient, int x, int y)
{
    int gx = 0;
    int gy = 0;
    size_t idx = y * image->columns + x;
    for (int dy = -1; dy < 2; dy++)
    {
        for (int dx = -1; dx < 2; dx++)
        {
            int id_sobel = 3 * (dy + 1) + dx + 1;
            int grey_value = pixel_gray(image, x + dx, y + dy);
            gx += grey_value * sobel_x[id_sobel];
            gy += grey_value * sobel_y[id_sobel];
        }
    }
    gradient[idx] = gx * gx + gy * gy;
}

static inline void compute_gradient(Image *image, uint64_t *gradient)
{
    int rows = image->rows;
    int columns = image->columns;
    for (int y = 1; y < rows - 1; y++)
    {
        for (int x = 1; x < columns - 1; x++)
        {
            compute_sobel(image, gradient, x, y);
        }
    }
}

static inline void sample_points(Image *image, Point2i *points, size_t number_of_points)
{
    int rows = image->rows;
    int columns = image->columns;
    points[0] = (Point2i){0, 0};
    points[1] = (Point2i){columns - 1, 0};
    points[2] = (Point2i){0, rows - 1};
    points[3] = (Point2i){columns - 1, rows - 1};
    uint64_t *gradient = (uint64_t *)malloc(sizeof(uint64_t) * rows * columns);
    memset(gradient, 0, sizeof(uint64_t) * columns * rows);
    uint64_t *cdf = (uint64_t *)malloc(sizeof(uint64_t) * rows * columns);
    compute_gradient(image, gradient);
    cdf[0] = gradient[0];
    for (int k = 1; k < columns * rows; k++)
    {
        cdf[k] = gradient[k] + cdf[k - 1];
    }
    int total = cdf[columns * rows - 1];
    for (int k = 4; k < number_of_points; k++)
    {
        uint64_t sample = fast_rand_range_mul(total);
        size_t index = binary_search(cdf, sample, columns * rows - 1);
        points[k].x = index % columns;
        points[k].y = index / columns;
    }
    free(gradient);
    free(cdf);
}

#endif
