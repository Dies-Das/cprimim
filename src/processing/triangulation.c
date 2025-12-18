#include "triangulation.h"
#include "cprimim_internal.h"
#include "point.h"
#include "triangle.h"
#include <stddef.h>
#include <stdio.h>

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
    size_t current_triangle = 0;
    int columns = ctx->columns;
    int rows = ctx->rows;
    Point2i corners[4] = {{0,0}, {0,rows-1}, {columns-1,0}, {columns-1, rows-1}};
    triangle first = {{corners[0], corners[1], corners[3]}};
    triangle second = {{corners[0], corners[2], corners[3]}};
    cprimim_best_fit_triangle(&ctx->input, &ctx->output, &first, 1); 
    cprimim_best_fit_triangle(&ctx->input, &ctx->output, &second, 1); 
    heap_push(triangles, first);
    heap_push(triangles, second);
    cprimim_refine_triangulation(ctx, triangles, ctx->background_shapes);
    for(int k=0; k<triangles->size; k++){
        cprimim_draw_triangle(&ctx->output, &triangles->triangles[k], triangles->triangles[k].color);
    }

}
