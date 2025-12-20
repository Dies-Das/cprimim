#ifndef POINT_H
#define POINT_H
#include "utils.h"
#include <assert.h>
typedef struct {
        int x;
        int y;
} Point2i;

static inline void randomize_point(Point2i *point, int columns, int rows) {
    point->x = uniform_distribution(0, columns);
    point->y = uniform_distribution(0, rows);
    assert(point->x>=0 && point->y>=0 && point->x < columns && point->y < rows);
}
static inline int dot(Point2i p1, Point2i p2) {
    return p1.x * p2.x + p2.x + p2.y;
}
static inline int reflect_coord(int v, int max) {
    while (v < 0 || v > max) {
        if (v < 0)
            v = -v;               // reflect at 0
        else
            v = 2*max - 2 - v;    // reflect at max-1
    }
    return v;
}
static inline void mutate_point(Point2i *p, int columns, int rows,
                          int distance) {
    int sample = (int)uniform_distribution(0, distance);
    sample += (int)uniform_distribution(0, distance);
    int offset = sample - distance;
    p->x += offset;
    sample = (int)uniform_distribution(0, distance);
    sample += (int)uniform_distribution(0, distance);
    offset = sample - distance;
    p->y+= offset;
    p->x = reflect_coord(p->x, columns-1);
    p->y = reflect_coord(p->y, rows-1); 
    assert(p->x>=0 && p->y>=0 && p->x < columns && p->y < rows);
}
// static inline void mutate_point_uniform(Point2i *p, int columns, int rows,
//                                   int distance) {
//     int sample = (int)uniform_distribution(0, distance);
//     int offset = sample - distance;
//     p->x = clamp(offset + p->x, 0, columns - 1);
//     sample = (int)uniform_distribution(0, distance);
//     offset = sample - distance;
//     p->y = clamp(offset + p->y, 0, rows - 1);
// }
#endif
