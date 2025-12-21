#include "line.h"
#include "color.h"
#include "optimize.h"
#include "point.h"
#include "utils.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LINE_PIXEL_ITERATOR(FUNC_NAME, CALLBACK)                                                   \
    static void FUNC_NAME(Image *image, line *line, void *payload)                                 \
    {                                                                                              \
        int x0 = line->points[0].x;                                                                \
        int x1 = line->points[1].x;                                                                \
        int y0 = line->points[0].y;                                                                \
        int y1 = line->points[1].y;                                                                \
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;                                              \
        int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;                                              \
        int err = dx - dy ;                                                             \
                                                                                                   \
        int half = (line->thickness + 1) / 2;                                                      \
                                                                                                   \
        bool steep = (dy > dx);                                                                    \
                                                                                                   \
        while (true)                                                                               \
        {                                                                                          \
            if (steep)                                                                             \
            {                                                                                      \
                for (int off = -half; off <= half; off++)                                          \
                {                                                                                  \
                    int xx = x0 + off;                                                             \
                    int yy = y0;                                                                   \
                    if (xx >= 0 && yy >= 0 && xx < image->columns && yy < image->rows)             \
                    {                                                                              \
                        CALLBACK(image, xx, yy, payload);                                          \
                    }                                                                              \
                }                                                                                  \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                for (int off = -half; off <= half; off++)                                          \
                {                                                                                  \
                    int xx = x0;                                                                   \
                    int yy = y0 + off;                                                             \
                    if (xx >= 0 && yy >= 0 && xx < image->columns && yy < image->rows)             \
                    {                                                                              \
                        CALLBACK(image, xx, yy, payload);                                          \
                    }                                                                              \
                }                                                                                  \
            }                                                                                      \
                                                                                                   \
            if (x0 == x1 && y0 == y1)                                                              \
                break;                                                                             \
                                                                                                   \
            int e2 = err * 2;                                                                      \
            if (e2 > -dy)                                                                          \
            {                                                                                      \
                err -= dy;                                                                         \
                x0 += sx;                                                                          \
            }                                                                                      \
            if (e2 < dx)                                                                           \
            {                                                                                      \
                err += dx;                                                                         \
                y0 += sy;                                                                          \
            }                                                                                      \
        }                                                                                          \
        return;                                                                                    \
    }
static line random_line(int seed_index, int n_init, int columns, int rows, int mutation_radius)
{
    int G = (int)ceil(sqrt((double)n_init));
    int cell_x = seed_index % G;
    int cell_y = seed_index / G;
    int cell_w = columns / G;
    int cell_h = rows / G;

    line bz = {0};

    int x0 = cell_x * cell_w + uniform_distribution(0, cell_w);
    int y0 = cell_y * cell_h + uniform_distribution(0, cell_h);
    bz.thickness = uniform_distribution(1, 4);
    bz.points[0].x = x0;
    bz.points[0].y = y0;
    bz.points[1] = bz.points[0];
    mutate_point(&bz.points[1], columns, rows, mutation_radius);
    // mutate_line(&bz, columns, rows);
    return bz;
}

LINE_PIXEL_ITERATOR(improvement, compare_pixel_callback)
LINE_PIXEL_ITERATOR(draw, draw_pixel_callback)
SORT(line)
BEST_FIT(line)
BEST_INITIAL(line)

static void draw_line(Image *image, line *line, Color color)
{
    DrawData data = {0};
    data.color = &color;
    data.output = image;
    draw(image, line, &data);
}
static void mutate_line(line *line, int columns, int rows, int mutation_radius)
{
    int index = fast_rand() & 1;
    mutate_point(&line->points[index], columns, rows, mutation_radius);
    index = fast_rand() & 1;
    if (index)
    {
        line->thickness += uniform_distribution(-1, 2);
        line->thickness = clamp(line->thickness, 1, 8);
    }
}

#define COARSE_STRIDE 2
SHAPE_APPROX(line)
void cprimim_write_line_svg(FILE *file, line *line, double alpha)
{
    fprintf(file, "<line x1=\"%i\" x2=\"%i\" y1=\"%i\" y2=\"%i\"", line->points[0].x,
            line->points[1].x, line->points[0].y, line->points[1].y);
    fprintf(file, " stroke=\"rgb(%u,%u,%u)\" stroke-opacity=\"%f\" stroke-width=\"%i\" stroke-linecap=\"round\"/>",
            line->color.r, line->color.g, line->color.b, alpha, line->thickness);
}
