#ifndef TRIANGULATION_H
#define TRIANGULATION_H
#include "cprimim_internal.h"
#include <stddef.h>

void cprimim_set_triangulation(cprimim_Context *ctx);

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
#endif
