#ifndef IMAGE_INTERNAL_H
#define IMAGE_INTERNAL_H
#include "image.h"
typedef struct {
        int improvement;
        cprimim_Image *other;
        cprimim_Color *color;
} cprimim_Comparator;
typedef struct {
        uint64_t *indices;
        uint32_t count;
} cprimim_IndexBuffer;

#endif // !IMAGE_INTERNAL_H
