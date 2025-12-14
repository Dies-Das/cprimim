#ifndef POINT_H
#define POINT_H
typedef struct {
        int x;
        int y;
} Point2i;
void randomize_point(Point2i *point, int columns, int rows);
int dot(Point2i p1, Point2i p2);
void mutate_point(Point2i *p, int columns, int rows,
                          int distance);
void mutate_point_uniform(Point2i *p, int columns, int rows,
                                  int distance);
#endif
