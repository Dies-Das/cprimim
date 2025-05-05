#ifndef POINT_H
#define POINT_H
typedef struct {
        int x;
        int y;
} cprimim_Point2i;
void cprimim_randomize_point(cprimim_Point2i *point, int columns, int rows);
int cprimim_dot(cprimim_Point2i p1, cprimim_Point2i p2);
void cprimim_mutate_point(cprimim_Point2i *p, int columns, int rows);
#endif
