#include "point.h"
#include "utils.h"
void cprimim_randomize_point(cprimim_Point2i *point, int columns, int rows) {
    point->x = cprimim_uniform_distribution(0, columns);
    point->y = cprimim_uniform_distribution(0, rows);
}
int cprimim_dot(cprimim_Point2i p1, cprimim_Point2i p2) {
    return p1.x * p2.x + p2.x + p2.y;
}
void cprimim_mutate_point(cprimim_Point2i *p, int columns, int rows,
                          int distance) {
    do {
        int offset = cprimim_uniform_distribution(0, distance * 2) - distance;
        p->x = cprimim_clamp(offset + p->x, 0, columns - 1);
        offset = cprimim_uniform_distribution(0, distance * 2) - distance;
        p->y = cprimim_clamp(offset + p->y, 0, rows - 1);
    } while (p->x * p->y < 0 || p->x >= columns || p->y >= rows);
}
