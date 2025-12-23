#ifndef TRIANGLE_H
#define TRIANGLE_H
#include "color.h"
#include "cprimim_internal.h"
#include "point.h"
void cprimim_triangle_approx(cprimim_Context *context);

void cprimim_write_triangle_svg(FILE *file, triangle *triangle, double alpha);

void cprimim_best_fit_triangle(Image *image, Image *output, triangle *shape, int stride, uint8_t alpha);
void cprimim_draw_triangle(CairoContext* cairo, triangle *triangle);
#endif // !TRIANGLE_H
