#ifndef TRIANGLE_H
#define TRIANGLE_H
#include "color.h"
#include "image.h"
#include "point.h"
#include <stdbool.h>
typedef struct {
        cprimim_Point2i points[3];
} cprimim_Triangle;
void cprimim_draw_triangle(cprimim_Image *image, cprimim_Triangle triangle,
                           cprimim_Color color, double thickness);
void cprimim_randomize_triangle(cprimim_Triangle *triangle, int columns,
                                int rows);
cprimim_Color cprimim_average_color_triangle(cprimim_Image *image,
                                             cprimim_Triangle triangle,
                                             double thickness);
#endif // !TRIANGLE_H
