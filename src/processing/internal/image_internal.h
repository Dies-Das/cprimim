#ifndef IMAGE_INTERNAL_H
#define IMAGE_INTERNAL_H
#include "color.h"
#include "image.h"
typedef struct {
        int improvement;
        cprimim_Image *other;
        int64_t sum_diffs[3];
        int64_t sum_diffs_squared[3];
        int64_t error_old;
        int counter;
} cprimim_Comparator;
typedef struct {
        cprimim_Image *output;
        cprimim_Color *color;
} cprimim_DrawData;
#endif // !IMAGE_INTERNAL_H
