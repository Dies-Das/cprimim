#ifndef LINE_H
#define LINE_H
#include "color.h"
#include "cprimim_internal.h"
#include "image_internal.h"
#include "point.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
        cprimim_Point2i points[2];
	int64_t improvement;
	int64_t improvement_coarse;
	cprimim_Color color;
} cprimim_line;
void cprimim_line_approx(cprimim_Context * context);

#endif // !LINE_H
