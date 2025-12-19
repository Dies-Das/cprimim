#include "delaunay.h"
#include "background.h"
#include "point.h"
#include "sample.h"
#include "triangle.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct
{
    Point2i points[3];
    bool bad;
} DelTriangle;
typedef struct
{
    Point2i points[2];
} Edge;
typedef struct
{
    Point2i c;
    double r;
} Circle;
void generate_triangulation(Point2i *points, size_t number_of_points, Triangulation *triangulation);
static inline bool has_super(DelTriangle *triangle);
static inline bool inside_circumcircle(Point2i *point, DelTriangle *triangle);
static inline bool not_in_bad_triangles(DelTriangle *triangles, size_t current_triangle, Edge edge,
                                        size_t number_of_triangles);
static inline Edge get_edge(DelTriangle *triangle, size_t index);
static Point2i SUPER_TRIANGLE[3] = {{-10000, -10000}, {20000, -10000}, {5000, 20000}};
static inline int64_t cross2d(Point2i a, Point2i b, Point2i c)
{
    return (int64_t)(b.x - a.x) * (c.y - a.y) - (int64_t)(b.y - a.y) * (c.x - a.x);
}

void cprimim_delaunay_triangulation(cprimim_Context *ctx)
{
    Point2i *points = malloc(sizeof(Point2i) * ctx->background_shapes);
    sample_points(&ctx->input, points, ctx->background_shapes);
    generate_triangulation(points, ctx->background_shapes, &ctx->state.background.triag);
    Triangulation* triangles = &ctx->state.background.triag;
    for(int k=0; k<triangles->size; k++){
        cprimim_best_fit_triangle(&ctx->input, &ctx->output, &triangles->triangles[k], 1);
        cprimim_draw_triangle(&ctx->output, &triangles->triangles[k], triangles->triangles[k].color);
    }
    free(points);
}
DelTriangle make_triangle(Point2i p0, Point2i p1, Point2i p2)
{
    if (cross2d(p0, p1, p2) < 0)
    {
        return (DelTriangle){.points = {p0, p2, p1}, .bad = false};
    }
    return (DelTriangle){.points = {p0, p1, p2}, .bad = false};
}
void generate_triangulation(Point2i *points, size_t number_of_points, Triangulation *triangulation)
{
    printf("number of points is %lu\n", number_of_points);
    DelTriangle *triangles = (DelTriangle *)malloc(sizeof(DelTriangle) * 3 * number_of_points);
    Edge *polygon = (Edge *)malloc(sizeof(Edge) * 10*number_of_points);
    triangles[0] = make_triangle(SUPER_TRIANGLE[0], SUPER_TRIANGLE[1], SUPER_TRIANGLE[2]);
    size_t n_triangles = 1;
    for (int k = 0; k < number_of_points; k++)
    {

        size_t n_poly_edges = 0;
        size_t bad_count = 0;
        for (size_t j = 0; j < n_triangles; j++)
        {
            if (triangles[j].bad)
                continue;

            if (inside_circumcircle(&points[k], &triangles[j]))
            {
                triangles[j].bad = true;
                bad_count++;
            }
        }
        printf("Point %d: marked %zu bad triangles\n", k, bad_count);
        for (size_t j = 0; j < n_triangles; j++)
        {
            if (triangles[j].bad)
            {
                for (size_t edge_id = 0; edge_id < 3; edge_id++)
                {
                    Edge current = get_edge(&triangles[j], edge_id);
                    if (not_in_bad_triangles(triangles, j, current, n_triangles))
                    {
                        polygon[n_poly_edges++] = current;
                    }
                }
            }
        }
        size_t new_size = 0;
        for(size_t tri_index = 0; tri_index < n_triangles; tri_index++){
            if(!triangles[tri_index].bad){
                triangles[new_size++] = triangles[tri_index];
            }
        }
        n_triangles = new_size;
        for (size_t edge_id = 0; edge_id < n_poly_edges; edge_id++)
        {
            Edge current = polygon[edge_id];
            triangles[n_triangles++] =
                make_triangle(current.points[0], current.points[1], points[k]);
        }
        printf("Point %d: %zu boundary edges, n_triangles now %zu\n", k, n_poly_edges, n_triangles);
    }
    for (int k = 0; k < n_triangles; k++)
    {
        if (!triangles[k].bad && !has_super(&triangles[k]))
        {
            for (int j = 0; j < 3; j++)
            {
                triangulation->triangles[triangulation->size].points[j] = triangles[k].points[j];
            }
            triangulation->size++;
        }
    }
    free(polygon);
    free(triangles);
    printf("triangulation size: %lu", triangulation->size);
}
static inline bool inside_circumcircle(Point2i *point, DelTriangle *triangle)
{
    int64_t ax = triangle->points[0].x - point->x;
    int64_t ay = triangle->points[0].y - point->y;
    int64_t bx = triangle->points[1].x - point->x;
    int64_t by = triangle->points[1].y - point->y;
    int64_t cx = triangle->points[2].x - point->x;
    int64_t cy = triangle->points[2].y - point->y;

    int64_t aa = ax * ax + ay * ay;
    int64_t bb = bx * bx + by * by;
    int64_t cc = cx * cx + cy * cy;

    int64_t det = ax * (by * cc - cy * bb) - ay * (bx * cc - cx * bb) + aa * (bx * cy - cx * by);

    return det > 0; // Assumes CCW winding
}
static inline bool not_in_bad_triangles(DelTriangle *triangles, size_t current_triangle, Edge edge,
                                        size_t number_of_triangles)
{
    for (size_t k = 0; k < number_of_triangles; k++)
    {
        if (!triangles[k].bad || current_triangle == k)
            continue;
        bool first = false;
        bool second = false;
        for (int j = 0; j < 3; j++)
        {
            if (triangles[k].points[j].x == edge.points[0].x &&
                triangles[k].points[j].y == edge.points[0].y)
            {
                first = true;
            }
            if (triangles[k].points[j].x == edge.points[1].x &&
                triangles[k].points[j].y == edge.points[1].y)
            {
                second = true;
            }
        }
        if (first && second)
        {
            return false;
        }
    }
    return true;
}
static inline Edge get_edge(DelTriangle *triangle, size_t index)
{
    return (Edge){triangle->points[index], triangle->points[(index + 1) % 3]};
}

static inline bool has_super(DelTriangle *triangle)
{
    for (int k = 0; k < 3; k++)
    {
        for (int j = 0; j < 3; j++)
        {
            if ((triangle->points[k].x == SUPER_TRIANGLE[j].x) &&
                (triangle->points[k].y == SUPER_TRIANGLE[j].y))
                return true;
        }
    }
    return false;
}
