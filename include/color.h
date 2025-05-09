#ifndef COLOR_H
#define COLOR_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
} cprimim_Color;
typedef struct {
        size_t r;
        size_t g;
        size_t b;
        int count;
} cprimim_AvgColor;
#endif // !COLOR_H
