#ifndef SHAPES_H
#define SHAPES_H
#include "color.h"
#include "point.h"

typedef struct
{
    Point2i points[2];
    int64_t improvement;
    int64_t residual;
    int64_t improvement_coarse;
    uint8_t thickness;
    Color color;
} line;

typedef struct
{
    Point2i points[3];
    int64_t improvement;
    int64_t residual;
    int64_t improvement_coarse;
    Color color;

} bezier;
typedef struct
{
    Point2i points[3];
    int64_t improvement;
    int64_t residual;
    int64_t improvement_coarse;
    Color color;
} triangle;
typedef enum
{
    LINE,
    BEZIER,
    TRIANGLE,
    RECTANGLE,
    ELLIPSE
} ShapeType;
typedef struct
{
    ShapeType s;
    union
    {
        line line;
        bezier bezier;
        triangle triangle;
    } shape;
} shape;

#endif
