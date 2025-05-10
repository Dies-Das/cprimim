#ifndef LINE_H
#define LINE_H
#include "color.h"
#include "image_internal.h"
#include "point.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
        cprimim_Point2i points[2];
} cprimim_Line;
void cprimim_draw_line(cprimim_Image *image, cprimim_IndexBuffer *buffer,
                       cprimim_Line line, cprimim_Color color,
                       double thickness);
void cprimim_randomize_line(cprimim_Line *line, int columns, int rows);
#endif // !LINE_H
