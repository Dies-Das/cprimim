#ifndef TRIANGLE_H
#define TRIANGLE_H
#include "color.h"
#include "cprimim_internal.h"
#include "point.h"
void cprimim_triangle_approx(cprimim_Context *context);

void cprimim_write_triangle_svg(FILE *file, triangle *triangle);

void cprimim_best_fit_triangle(Image *image, Image *output, triangle *shape, int stride);
void cprimim_draw_triangle(Image *image, triangle *triangle, Color color);
#endif // !TRIANGLE_H
