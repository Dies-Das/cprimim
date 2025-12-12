#ifndef TRIANGLE_H
#define TRIANGLE_H
#include "color.h"
#include "cprimim_internal.h"
#include "point.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
        cprimim_Point2i points[3];
	int64_t improvement;
    int determinant;
	cprimim_Color color;
} cprimim_triangle;
void cprimim_triangle_approx(cprimim_Context * context);

#endif // !TRIANGLE_H
