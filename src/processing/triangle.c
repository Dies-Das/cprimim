#include "triangle.h"
#include "color.h"
#include "cprimim_internal.h"
#include "optimize.h"
#include "point.h"
#include "utils.h"
#include <assert.h>
#include <cairo/cairo.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void draw_triangle_cairo(CairoContext* cairo, triangle *trig)
{
    Color color = trig->color;
    cairo_set_line_width(cairo->cr, (double)trig->thickness);

    cairo_set_source_rgba(cairo->cr, u8_to_unit(color.r), u8_to_unit(color.g), u8_to_unit(color.b),
                          u8_to_unit(color.alpha)); 

    cairo_new_path(cairo->cr);
    cairo_move_to(cairo->cr, trig->points[0].x + 0.5, trig->points[0].y + 0.5);
    cairo_line_to(cairo->cr, trig->points[1].x + 0.5, trig->points[1].y + 0.5);
    cairo_line_to(cairo->cr, trig->points[2].x + 0.5, trig->points[2].y + 0.5);
    cairo_close_path(cairo->cr);
    cairo_fill(cairo->cr);


    cairo_surface_flush(cairo->surf);
}
typedef struct
{
    Point2i points[2];
} Edge;

static int get_determinant(Point2i point1, Point2i point2, Point2i point3)
{
    Point2i ab = {point2.x - point1.x, point2.y - point1.y};
    Point2i ac = {point3.x - point1.x, point3.y - point1.y};
    return ab.y * ac.x - ab.x * ac.y;
}
#define TRIANGLE_PIXEL_ITERATOR(FUNC_NAME, CALLBACK)                                               \
    void FUNC_NAME(Image *image, triangle *triangle, void *payload)                         \
    {                                                                                              \
        Point2i p[3] = {triangle->points[0], triangle->points[1], triangle->points[2]};            \
        /* Sort by y */                                                                            \
        if (p[0].y > p[1].y)                                                                       \
        {                                                                                          \
            Point2i t = p[0];                                                                      \
            p[0] = p[1];                                                                           \
            p[1] = t;                                                                              \
        }                                                                                          \
        if (p[1].y > p[2].y)                                                                       \
        {                                                                                          \
            Point2i t = p[1];                                                                      \
            p[1] = p[2];                                                                           \
            p[2] = t;                                                                              \
        }                                                                                          \
        if (p[0].y > p[1].y)                                                                       \
        {                                                                                          \
            Point2i t = p[0];                                                                      \
            p[0] = p[1];                                                                           \
            p[1] = t;                                                                              \
        }                                                                                          \
                                                                                                   \
        int y0 = p[0].y, y1 = p[1].y, y2 = p[2].y;                                                 \
        int x0 = p[0].x, x1 = p[1].x, x2 = p[2].x;                                                 \
                                                                                                   \
        if (y0 == y2)                                                                              \
            return;                                                                                \
                                                                                                   \
        for (int y = y0; y <= y2; y++)                                                             \
        {                                                                                          \
            int x_long = x0 + (x2 - x0) * (y - y0) / (y2 - y0);                                    \
            int x_short;                                                                           \
            if (y < y1)                                                                            \
            {                                                                                      \
                if (y1 == y0)                                                                      \
                    continue;                                                                      \
                x_short = x0 + (x1 - x0) * (y - y0) / (y1 - y0);                                   \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                if (y2 == y1)                                                                      \
                    continue;                                                                      \
                x_short = x1 + (x2 - x1) * (y - y1) / (y2 - y1);                                   \
            }                                                                                      \
            int xl = x_long < x_short ? x_long : x_short;                                          \
            int xr = x_long < x_short ? x_short : x_long;                                          \
            for (int x = xl; x <= xr; x++)                                                         \
            {                                                                                      \
                CALLBACK(image, x, y, payload);                                                    \
            }                                                                                      \
        }                                                                                          \
    }
static void mutate_triangle(triangle *input, int columns, int rows, int mutation_radius)
{
    do
    {

        uint64_t random_value = fast_rand();
        uint64_t index = random_value % 3;
        mutate_point(&input->points[index], columns, rows, mutation_radius);
    } while (get_determinant(input->points[0], input->points[1], input->points[2]) == 0);
    // if(input->determinant<0){
    //     Point2i temp = input->points[0];
    //     input->points[0] = input->points[1];
    //
    //     input->points[1] = temp;
    //     input->determinant *= -1;
    // }
}

static triangle random_triangle(int seed_index, int n_init, int columns, int rows,
                                int mutation_radius)
{
    int G = (int)ceil(sqrt((double)n_init));
    int cell_x = seed_index % G;
    int cell_y = seed_index / G;
    int cell_w = columns / G;
    int cell_h = rows / G;

    triangle triangle = {0};

    do
    {
        int x0 = cell_x * cell_w + uniform_distribution(0, cell_w);
        int y0 = cell_y * cell_h + uniform_distribution(0, cell_h);
        triangle.points[0].x = x0;
        triangle.points[0].y = y0;
        triangle.points[1] = triangle.points[0];
        mutate_point(&triangle.points[1], columns, rows, mutation_radius);
        triangle.points[2] = triangle.points[1];
        mutate_point(&triangle.points[2], columns, rows, mutation_radius);
    } while (get_determinant(triangle.points[0], triangle.points[1], triangle.points[2]) == 0);
    // mutate_triangle(&bz, columns, rows);
    // if(triangle.determinant<0){
    //     Point2i temp = triangle.points[0];
    //     triangle.points[0] = triangle.points[1];
    //
    //     triangle.points[1] = temp;
    //     triangle.determinant *= -1;
    // }
    return triangle;
}
TRIANGLE_PIXEL_ITERATOR(improvement, compare_pixel_callback)
// TRIANGLE_PIXEL_ITERATOR(draw, draw_pixel_callback)
void cprimim_draw_triangle(CairoContext* cairo, triangle *triangle)
{
    // DrawData data = {0};
    // data.color = &color;
    // data.output = image;
    draw_triangle_cairo(cairo, triangle);
}
#define draw_triangle cprimim_draw_triangle 
SORT(triangle)

BEST_FIT(triangle)

BEST_INITIAL(triangle)

#define COARSE_STRIDE 2
SHAPE_APPROX(triangle)

void cprimim_write_triangle_svg(FILE *file, triangle *triangle, double alpha)
{
    fprintf(file, "<polygon points=\"");
    for (int k = 0; k < 3; k++)
    {
        fprintf(file, "%i %i ", triangle->points[k].x, triangle->points[k].y);
    }
    fprintf(file, "\" fill=\"rgb(%u,%u,%u)\" opacity=\"%f\"/>", triangle->color.r,
            triangle->color.g, triangle->color.b, alpha);
}
