#ifndef LINE_H
#define LINE_H
#include "color.h"
#include "point.h"
#include "cprimim_internal.h"
void cprimim_line_approx(cprimim_Context * context);

void write_line_svg(FILE * file, cprimim_line* line);
#endif // !LINE_H
