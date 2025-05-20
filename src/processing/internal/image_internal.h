#ifndef IMAGE_INTERNAL_H
#define IMAGE_INTERNAL_H
#include "image.h"
typedef struct {
        int improvement;
        cprimim_Image *other;
        cprimim_Color *color;
} cprimim_Comparator;
typedef struct {
        cprimim_Image *output;
        cprimim_Color *color;
} cprimim_DrawData;
#endif // !IMAGE_INTERNAL_H
