#ifndef SAMPLE_H
#define SAMPLE_H
// #define UPDATE
#include "background.h"
#include "cprimim_internal.h"
#include "image_internal.h"
#include "point.h"
#include "utils.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static const int sobel_x[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
static const int sobel_y[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
#ifdef UPDATE
static void update_cdf(uint64_t *gradient, uint64_t *cdf, size_t index, int columns, int rows,
                       int min_dist);
#endif
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

static inline size_t sample_points(Image *image, Point2i *points, size_t number_of_points)
{
    int rows = image->rows;
    int columns = image->columns;
    int min_dist = ceilf(sqrtf(columns * rows / (float)number_of_points) * .7);
    min_dist = min_dist == 0 ? 1 : min_dist;
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
    uint64_t total = cdf[columns * rows - 1];
    size_t real_nr_points = 4;
    for (size_t k = 4; k < number_of_points && cdf[columns * rows - 1] > 0; k++)
    {
        uint64_t sample = fast_rand() % (total);
        size_t index = binary_search(cdf, sample, columns * rows - 1);
        points[k].x = index % columns;
        points[k].y = index / columns;
#ifdef UPDATE
        update_cdf(gradient, cdf, index, columns, rows, min_dist);
#endif
        real_nr_points++;
    }

    free(gradient);
    free(cdf);
    return real_nr_points;
}
#ifdef UPDATE
static void update_cdf(uint64_t *gradient, uint64_t *cdf, size_t index, int columns, int rows,
                       int min_dist)
{
    int x = index % columns;
    int y = index / columns;
    int dx = min_dist;
    for (int ky = -dx; ky <= dx; ky++)
    {
        int py = y + ky;
        if (py < 0 || py >= rows)
            continue;
        for (int kx = -dx; kx <= dx; kx++)
        {
            int px = x + kx;
            if (px < 0 || px >= columns)
                continue;
            gradient[py * columns + px] = 0;
        }
    }
    cdf[0] = gradient[0];
    for (int k = 1; k < columns * rows; k++)
    {
        cdf[k] = gradient[k] + cdf[k - 1];
    }
}
#endif
#endif
