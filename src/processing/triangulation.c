#include "triangulation.h"
#include "cprimim_internal.h"
#include "point.h"
#include "triangle.h"
#include <stddef.h>
#include <stdio.h>


static inline void heap_swap(triangle *a, triangle *b)
{
    triangle temp = *a;
    *a = *b;
    *b = temp;
}

static inline void sift_up(Triangulation *heap, size_t element)
{
    while (element > 0)
    {
        size_t parent = (element - 1) / 2;
        if (heap->triangles[element].residual <= heap->triangles[parent].residual)
            break;
        heap_swap(&heap->triangles[element], &heap->triangles[parent]);
        element = parent;
    }
}

static inline void sift_down(Triangulation *heap, size_t element)
{
    while (2 * element + 1 < heap->size)
    {
        size_t left = 2 * element + 1;
        size_t right = 2 * element + 2;
        size_t largest_residual = element;
        if (left < heap->size &&
            heap->triangles[left].residual > heap->triangles[largest_residual].residual)
        {
            largest_residual = left;
        }
        if (right < heap->size &&
            heap->triangles[right].residual > heap->triangles[largest_residual].residual)
        {
            largest_residual = right;
        }
        if (largest_residual == element)
            break;
        heap_swap(&heap->triangles[element], &heap->triangles[largest_residual]);
        element = largest_residual;
    }
}

static void heap_push(Triangulation *heap, triangle t)
{
    heap->triangles[heap->size] = t;
    sift_up(heap, heap->size);
    heap->size++;
}

static triangle heap_pop(Triangulation *heap)
{
    triangle top = heap->triangles[0];
    heap->triangles[0] = heap->triangles[heap->size - 1];
    heap->size--;
    if (heap->size > 0)
        sift_down(heap, 0);
    return top;
}
static inline Point2i midpoint(Point2i p1, Point2i p2){
    Point2i midpoint = p1;
    midpoint.x += p2.x;
    midpoint.y += p2.y;
    midpoint.x /=2;
    midpoint.y /=2;
    return midpoint;
}


void cprimim_refine_triangulation(cprimim_Context* ctx, Triangulation* heap, size_t nr_of_triangles){
    while(heap->size+3 <= nr_of_triangles){
        triangle parent = heap_pop(heap);
        Point2i m01 = midpoint(parent.points[0], parent.points[1]);
        Point2i m12 = midpoint(parent.points[2], parent.points[1]);
        Point2i m02 = midpoint(parent.points[0], parent.points[2]);
        triangle children[4] ={
            {.points = {parent.points[0], m01, m02}},
            {.points = {parent.points[1], m01, m12}},
            {.points = {parent.points[2], m12, m02}},
            {.points = {m12, m01, m02}}
        };
        for(int k=0; k<4; k++){
            cprimim_best_fit_triangle(&ctx->input, &ctx->output, &children[k], 1); 
            heap_push(heap, children[k]);
        }
    }
}

void cprimim_set_triangulation(cprimim_Context *ctx){
    Triangulation *triangles = &ctx->state.background.triag;
    int columns = ctx->columns;
    int rows = ctx->rows;
    Point2i corners[4] = {{0,0}, {0,rows-1}, {columns-1,0}, {columns-1, rows-1}};
    triangle first = {{corners[0], corners[1], corners[3]},0, 0, 0, {0}};
    triangle second = {{corners[0], corners[2], corners[3]},0, 0, 0, {0}};
    cprimim_best_fit_triangle(&ctx->input, &ctx->output, &first, 1); 
    cprimim_best_fit_triangle(&ctx->input, &ctx->output, &second, 1); 
    heap_push(triangles, first);
    heap_push(triangles, second);
    cprimim_refine_triangulation(ctx, triangles, ctx->background_shapes);
    for(size_t k=0; k<triangles->size; k++){
        cprimim_draw_triangle(&ctx->output, &triangles->triangles[k], triangles->triangles[k].color);
    }

}
