#ifndef BEZIER_H
#define BEZIER_H
#include "image.h"
#include "point.h"
#include "utils.h"
typedef struct {
        cprimim_Point2i points[3];
} cprimim_Bezier;
void cprimim_bezier_approx(cprimim_Image *input, cprimim_Image *output,
                           int number_of_lines, int max_number_of_tries);
#endif // !BEZIER_H
