#include "triangle.h"
#include "color.h"
#include "optimize.h"
#include "point.h"
#include "utils.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
    Point2i points[2];
} Edge;

static Edge create_edge(Point2i p1, Point2i p2)
{
    if (p1.y > p2.y)
    {
        return (Edge){{p2, p1}};
    }
    else
    {
        return (Edge){{p1, p2}};
    }
}
static Point2i top_left(triangle *triangle)
{
    int minx = INT_MAX;
    int miny = INT_MAX;
    for (int k = 0; k < 3; k++)
    {
        if (triangle->points[k].y < miny)
        {
            miny = triangle->points[k].y;
        }
        if (triangle->points[k].x < minx)
        {
            minx = triangle->points[k].x;
        }
    }
    return (Point2i){minx, miny};
}
static Point2i bottom_right(triangle *triangle)
{
    int maxx = INT_MIN;
    int maxy = INT_MIN;
    for (int k = 0; k < 3; k++)
    {
        if (triangle->points[k].y > maxy)
        {
            maxy = triangle->points[k].y;
        }
        if (triangle->points[k].x > maxx)
        {
            maxx = triangle->points[k].x;
        }
    }
    return (Point2i){maxx, maxy};
}
static int get_determinant(Point2i point1, Point2i point2, Point2i point3)
{
    Point2i ab = {point2.x - point1.x, point2.y - point1.y};
    Point2i ac = {point3.x - point1.x, point3.y - point1.y};
    return ab.y * ac.x - ab.x * ac.y;
}
// #define TRIANGLE_PIXEL_ITERATOR(FUNC_NAME, CALLBACK)\
// static void FUNC_NAME(Image *image, triangle *triangle, \
//                                   void *payload) {\
//     Edge edges[3];\
//     for(int k=0; k<3; k++){\
//         edges[k] = create_edge(triangle->points[k], triangle->points[(k+1)%3]);\
// }\
//     Point2i bounding_box[2] = {top_left(triangle), bottom_right(triangle)};\
//     Point2i p0 = triangle->points[0];\
//     Point2i p1 = triangle->points[1];\
//     Point2i p2 = triangle->points[2];\
//     int edge_distances[3] = {0};\
//     edge_distances[0] = get_determinant(p1, p2, bounding_box[0]);\
//     edge_distances[1] = get_determinant(p2, p0, bounding_box[0]);\
//     edge_distances[2] = get_determinant(p0, p1, bounding_box[0]);\
// \
//     int dwdx[3] = {0};\
//     dwdx[0] = p1.y-p2.y;\
//     dwdx[1] = p2.y-p0.y;\
//     dwdx[2] = p0.y-p1.y;\
//     int dwdy[3] = {0};\
//     dwdy[0] = p1.x-p2.x;\
//     dwdy[1] = p2.x-p0.x;\
//     dwdy[2] = p0.x-p1.x;\
//     int w[3] = {0};\
//     int stop_early = 0;\
//     \
//     for(int y=bounding_box[0].y; y<=bounding_box[1].y; y++){\
//         for (int k=0; k<3; k++){\
//             w[k] = edge_distances[k];\
//         }\
//         for(int x=bounding_box[0].x; x<=bounding_box[1].x; x++){\
//             if((w[0]|w[1]|w[2])>=0){\
//                 (CALLBACK(image, x,y, payload)) ;\
//             }\
//             for (int k=0; k<3; k++){\
//                 w[k] -= dwdx[k];\
//             }\
//         }\
//         for (int k=0; k<3; k++){\
//             edge_distances[k] += dwdy[k];\
//         }\
// \
//     }\
// }
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
TRIANGLE_PIXEL_ITERATOR(draw, draw_pixel_callback)
void cprimim_draw_triangle(Image *image, triangle *triangle, Color color)
{
    DrawData data = {0};
    data.color = &color;
    data.output = image;
    draw(image, triangle, &data);
}
#define draw_triangle cprimim_draw_triangle 
SORT(triangle)

BEST_FIT(triangle)

BEST_INITIAL(triangle)

#define COARSE_STRIDE 2
SHAPE_APPROX(triangle)

void cprimim_write_triangle_svg(FILE *file, triangle *triangle)
{
    fprintf(file, "<polygon points=\"");
    for (int k = 0; k < 3; k++)
    {
        fprintf(file, "%i %i ", triangle->points[k].x, triangle->points[k].y);
    }
    fprintf(file, "\" fill=\"rgb(%u,%u,%u)\" opacity=\"0.5\"/>", triangle->color.r,
            triangle->color.g, triangle->color.b);
}
