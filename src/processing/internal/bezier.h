#ifndef BEZIER_H
#define BEZIER_H
#include "cprimim_internal.h"
#include "point.h"
#include "utils.h"
typedef struct {
        cprimim_Point2i points[3];
        int64_t improvement;
        cprimim_Color color;

} cprimim_bezier;
void cprimim_bezier_approx(cprimim_Context *context);
#endif // !BEZIER_H
