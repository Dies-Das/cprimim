#ifndef BEZIER_H
#define BEZIER_H
#include "point.h"
#include "color.h"
#include "cprimim_internal.h"
#include <stdio.h>
void cprimim_bezier_approx(cprimim_Context *context);
void cprimim_write_bezier_svg(FILE * file, bezier* bezier);
#endif // !BEZIER_H
