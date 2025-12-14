#ifndef TRIANGLE_H
#define TRIANGLE_H
#include "color.h"
#include "point.h"
#include "cprimim_internal.h"
void triangle_approx(cprimim_Context * context);

void write_triangle_svg(FILE * file, triangle* triangle);
#endif // !TRIANGLE_H
