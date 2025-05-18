#ifndef BEZIER_H
#define BEZIER_H
#include "cprimim.h"
#include "image.h"
#include "point.h"
#include "utils.h"
typedef struct {
        cprimim_Point2i points[3];
        int improvement;
        cprimim_Color color;
        // cprimim_IndexBuffer best_buffer;
        // cprimim_IndexBuffer buffer;

} cprimim_Bezier;
#if 0
void cprimim_bezier_approx(cprimim_Image *input, cprimim_Image *output,
                           int number_of_lines, int max_number_of_tries);
#else
void cprimim_bezier_approx(cprimim_Context *context);
#endif
#endif // !BEZIER_H
