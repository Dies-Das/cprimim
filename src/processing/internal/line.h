#ifndef LINE_H
#define LINE_H
#include "color.h"
#include "image.h"
#include "point.h"
#include <stdbool.h>
typedef struct {
        cprimim_Point2i points[2];
} cprimim_Line;
void cprimim_draw_line(cprimim_Image *image, cprimim_Line line,
                       cprimim_Color color, int thickness);
void cprimim_randomize_line(cprimim_Line *line, int columns, int rows);
cprimim_Color cprimim_average_color_line(const cprimim_Image *image,
                                         cprimim_Line line, int thickness);
#endif // !LINE_H
