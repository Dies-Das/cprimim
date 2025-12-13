#ifndef SHAPES_H
#define SHAPES_H
#include "point.h"
#include "color.h"

typedef struct {
        cprimim_Point2i points[2];
	int64_t improvement;
	int64_t improvement_coarse;
    uint8_t thickness;
	cprimim_Color color;
} cprimim_line;

typedef struct {
        cprimim_Point2i points[3];
        int64_t improvement;
	int64_t improvement_coarse;
        cprimim_Color color;

} cprimim_bezier;
typedef struct {
        cprimim_Point2i points[3];
	int64_t improvement;
	int64_t improvement_coarse;
    // int determinant;
	cprimim_Color color;
} cprimim_triangle;
typedef enum { LINE, BEZIER, TRIANGLE, RECTANGLE, ELLIPSE }cprimim_ShapeType ;
typedef struct{
    cprimim_ShapeType s;
    union{
        cprimim_line line;
        cprimim_bezier bezier;
        cprimim_triangle triangle;
    } shape;
} cprimim_shape;


#endif

