#ifndef BEZIER_H
#define BEZIER_H
#include "point.h"
#include "color.h"
#include "cprimim_internal.h"
#include <stdio.h>
void bezier_approx(cprimim_Context *context);
void write_bezier_svg(FILE * file, bezier* bezier);
#endif // !BEZIER_H
