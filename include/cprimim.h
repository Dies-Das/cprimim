#ifndef IMPOL_H
#define IMPOL_H
#include "image.h"
void cprimim_line_approx(const cprimim_Image *input, cprimim_Image *output,
                         int number_of_lines, int number_of_tries,
                         int thickness, cprimim_Image *output_buffer);
#endif // !IMPOL_H
