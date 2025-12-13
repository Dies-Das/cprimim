#ifndef BEZIER_H
#define BEZIER_H
#include "cprimim_internal.h"
#include "point.h"
#include "utils.h"
#include <stdio.h>
typedef struct {
        cprimim_Point2i points[3];
        int64_t improvement;
	int64_t improvement_coarse;
        cprimim_Color color;

} cprimim_bezier;
void cprimim_bezier_approx(cprimim_Context *context);
void write_bezier_svg(FILE * file, cprimim_bezier* bezier);
#endif // !BEZIER_H
