#ifndef BACKGROUND_H
#define BACKGROUND_H
#include "color.h"
#include "shapes.h"
typedef enum
{
    NONE,
    UNIFORM_TRIANGULATION,
    DELAUNAY
} cprimim_BackgroundType;

typedef struct
{
    triangle *triangles;
    size_t size;
} Triangulation;
typedef struct
{
    Color average;
    cprimim_BackgroundType bt;
    Triangulation triag;
} Background;
#endif
