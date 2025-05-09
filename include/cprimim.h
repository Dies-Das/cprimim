#ifndef IMPOL_H
#define IMPOL_H
#include "image.h"
void cprimim_line_approx(cprimim_Image *input, cprimim_Image *output,
                         int number_of_lines, int number_of_tries,
                         double thickness);
#endif // !IMPOL_H
