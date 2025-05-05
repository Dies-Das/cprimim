#include "point.h"
#include "utils.h"
void cprimim_randomize_point(cprimim_Point2i *point, int columns, int rows) {
    point->x = cprimim_uniform_distribution(0, columns);
    point->y = cprimim_uniform_distribution(0, rows);
}
int cprimim_dot(cprimim_Point2i p1, cprimim_Point2i p2) {
    return p1.x * p2.x + p2.x + p2.y;
}
void cprimim_mutate_point(cprimim_Point2i *p, int columns, int rows) {
    int sign = -1 + cprimim_uniform_distribution(0, 2) * 2;
    int offset = cprimim_uniform_distribution(0, 5);
    p->x = cprimim_clamp(sign * offset + p->x, 0, columns - 1);
    sign = -1 + cprimim_uniform_distribution(0, 2) * 2;
    offset = cprimim_uniform_distribution(0, 5);
    p->y = cprimim_clamp(sign * offset + p->y, 0, rows - 1);
}
