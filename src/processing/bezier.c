#include "bezier.h"
#include "optimize.h"
#include "color.h"
#include "point.h"
#include "utils.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

bool not_valid_bezier(cprimim_bezier *input) {
    bool valid = 0;
    int dx01 = abs(input->points[0].x - input->points[1].x);
    int dx02 = abs(input->points[0].x - input->points[2].x);
    int dx12 = abs(input->points[1].x - input->points[2].x);
    int dy01 = abs(input->points[0].y - input->points[1].y);
    int dy02 = abs(input->points[0].y - input->points[2].y);
    int dy12 = abs(input->points[1].y - input->points[2].y);
    int d01 = dx01 * dx01 + dy01 * dy01;
    int d02 = dx02 * dx02 + dy02 * dy02;
    int d12 = dx12 * dx12 + dy12 * dy12;
    valid = (d02 <= d12 || d02 <= d01);
    return valid;
}
void mutate_bezier(cprimim_bezier *input, int columns, int rows) {
    do {

        uint64_t random_value = fast_rand();
        uint64_t index = random_value % 3;
        cprimim_mutate_point(&input->points[index], columns, rows,
                             MUTATION_DISTANCE);
    } while (not_valid_bezier(input));
}

static cprimim_bezier random_bezier(int seed_index, int n_init,
                                               int columns, int rows) {

   int Gx = (int)floor(sqrt((double)n_init * (double)columns / (double)rows));
    if (Gx < 1) Gx = 1;
    int Gy = (n_init + Gx - 1) / Gx;  // ceil(n_init / Gx)

    int cell_x = seed_index % Gx;
    int cell_y = seed_index / Gx;

    int cell_w = columns / Gx;
    int cell_h = rows   / Gy;

    if (cell_w < 1) cell_w = 1;
    if (cell_h < 1) cell_h = 1;
    cprimim_bezier bz = {0};

    do {
        int x0 = cell_x * cell_w + cprimim_uniform_distribution(0, cell_w);
        int y0 = cell_y * cell_h + cprimim_uniform_distribution(0, cell_h);
        bz.points[0].x = x0;
        bz.points[0].y = y0;
        bz.points[1] = bz.points[0];
        cprimim_mutate_point_uniform(&bz.points[1], columns, rows,
                                     MUTATION_DISTANCE);
        bz.points[2] = bz.points[0];
        cprimim_mutate_point_uniform(&bz.points[2], columns, rows,
                                     MUTATION_DISTANCE);
    } while (not_valid_bezier(&bz));
    // mutate_bezier(&bz, columns, rows);
    return bz;
}

// Courtesy to http://members.chello.at/%7Eeasyfilter/Bresenham.pdf
#define LINE_SEG(FUNC_NAME, CALLBACK)                                          \
    static void FUNC_NAME(cprimim_Image *image, void *payload, int x0, int y0, \
                          int x1, int y1) {                                    \
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;                          \
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;                         \
        int err = dx + dy, e2; /* error value e_xy */                          \
        uint64_t index = 0;                                                    \
        for (;;) { /* loop */                                                  \
                                                                               \
            CALLBACK(image, x0, y0, payload);                                  \
            e2 = 2 * err;                                                      \
            if (e2 >= dy) { /* e_xy+e_x > 0 */                                 \
                if (x0 == x1)                                                  \
                    break;                                                     \
                err += dy;                                                     \
                x0 += sx;                                                      \
            }                                                                  \
            if (e2 <= dx) { /* e_xy+e_y < 0 */                                 \
                if (y0 == y1)                                                  \
                    break;                                                     \
                err += dx;                                                     \
                y0 += sy;                                                      \
            }                                                                  \
        }                                                                      \
    }
#define BEZIER_SEG(FUNC_NAME, CALLBACK)                                        \
    static void FUNC_NAME(                                                     \
        cprimim_Image *image, void *payload, int x0, int y0, int x1, int y1,   \
        int x2, int y2) { /* plot a limited quadratic bezier segment */        \
        int sx = x2 - x1, sy = y2 - y1;                                        \
        long xx = x0 - x1, yy = y0 - y1, xy; /* relative values for checks */  \
        int dx, dy, err, cur = xx * sy - yy * sx; /* curvature */              \
        uint64_t index = 0;                                                    \
        assert(xx *sx <= 0 &&                                                  \
               yy * sy <= 0); /* sign of gradient must not change */           \
        if (sx * (long)sx + sy * (long)sy >                                    \
            xx * xx + yy * yy) { /* begin with longer part */                  \
            x2 = x0;                                                           \
            x0 = sx + x1;                                                      \
            y2 = y0;                                                           \
            y0 = sy + y1;                                                      \
            cur = -cur; /* swap P0 P2 */                                       \
        }                                                                      \
        if (cur != 0) { /* no straight line */                                 \
            xx += sx;                                                          \
            xx *= sx = x0 < x2 ? 1 : -1; /* x step direction */                \
            yy += sy;                                                          \
            yy *= sy = y0 < y2 ? 1 : -1; /* y step direction */                \
            xy = 2 * xx * yy;                                                  \
            xx *= xx;                                                          \
            yy *= yy;                /* differences 2nd degree */              \
            if (cur * sx * sy < 0) { /* negated curvature? */                  \
                xx = -xx;                                                      \
                yy = -yy;                                                      \
                xy = -xy;                                                      \
                cur = -cur;                                                    \
            }                                                                  \
            dx = 4 * sy * cur * (x1 - x0) + xx -                               \
                 xy; /* differences 1st degree */                              \
            dy = 4 * sx * cur * (y0 - y1) + yy - xy;                           \
            xx += xx;                                                          \
            yy += yy;                                                          \
            err = dx + dy + xy; /* error 1st step */                           \
            do {                                                               \
                CALLBACK(image, x0, y0, payload);                              \
                if (x0 == x2 && y0 == y2)                                      \
                    return;        /* last pixel -> curve finished */          \
                y1 = 2 * err < dx; /* save value for test of y step */         \
                if (2 * err > dy) {                                            \
                    x0 += sx;                                                  \
                    dx -= xy;                                                  \
                    err += dy += yy;                                           \
                } /* x step */                                                 \
                if (y1) {                                                      \
                    y0 += sy;                                                  \
                    dy -= xy;                                                  \
                    err += dx += xx;                                           \
                } /* y step */                                                 \
            } while (dy < 0 &&                                                 \
                     dx > 0); /* gradient negates -> algorithm fails */        \
        }                                                                      \
        FUNC_NAME##_line(image, payload, x0, y0, x2,                           \
                         y2); /* plot remaining part to end */                 \
    }
#define BEZIER_PIXEL_ITERATOR(FUNC_NAME, CALLBACK)                             \
    LINE_SEG(FUNC_NAME##_seg_line, CALLBACK)                                   \
    BEZIER_SEG(FUNC_NAME##_seg, CALLBACK)                                      \
    static void FUNC_NAME(                                                     \
        cprimim_Image *image, cprimim_bezier *bezier,                          \
        void *payload) { /* plot any quadratic bezier curve */                 \
        int x0 = bezier->points[0].x;                                          \
        int x1 = bezier->points[1].x;                                          \
        int x2 = bezier->points[2].x;                                          \
        int y0 = bezier->points[0].y;                                          \
        int y1 = bezier->points[1].y;                                          \
        int y2 = bezier->points[2].y;                                          \
        int x = x0 - x1, y = y0 - y1;                                          \
        float t = x0 - 2 * x1 + x2, r;                                         \
        if ((long)x * (x2 - x1) > 0) {   /* horizontal cut at P4? */           \
            if ((long)y * (y2 - y1) > 0) /* vertical cut at P6 too? */         \
                if (fabs((y0 - 2 * y1 + y2) / t * x) >                         \
                    abs(y)) { /* which first? */                               \
                    x0 = x2;                                                   \
                    x2 = x + x1;                                               \
                    y0 = y2;                                                   \
                    y2 = y + y1; /* swap points */                             \
                } /* now horizontal cut at P4 comes first */                   \
            t = (x0 - x1) / t;                                                 \
            r = (1 - t) * ((1 - t) * y0 + 2.0 * t * y1) +                      \
                t * t * y2;                          /* By(t=P4) */            \
            t = (x0 * x2 - x1 * x1) * t / (x0 - x1); /* gradient dP4/dx=0 */   \
            x = floor(t + 0.5);                                                \
            y = floor(r + 0.5);                                                \
            r = (y1 - y0) * (t - x0) / (x1 - x0) +                             \
                y0; /* intersect P3 | P0 P1 */                                 \
            FUNC_NAME##_seg(image, payload, x0, y0, x, floor(r + 0.5), x, y);  \
            r = (y1 - y2) * (t - x2) / (x1 - x2) +                             \
                y2; /* intersect P4 | P1 P2 */                                 \
            x0 = x1 = x;                                                       \
            y0 = y;                                                            \
            y1 = floor(r + 0.5); /* P0 = P4, P1 = P8 */                        \
        }                                                                      \
        if ((long)(y0 - y1) * (y2 - y1) > 0) { /* vertical cut at P6? */       \
            t = y0 - 2 * y1 + y2;                                              \
            t = (y0 - y1) / t;                                                 \
            r = (1 - t) * ((1 - t) * x0 + 2.0 * t * x1) +                      \
                t * t * x2;                          /* Bx(t=P6) */            \
            t = (y0 * y2 - y1 * y1) * t / (y0 - y1); /* gradient dP6/dy=0 */   \
            x = floor(r + 0.5);                                                \
            y = floor(t + 0.5);                                                \
            r = (x1 - x0) * (t - y0) / (y1 - y0) +                             \
                x0; /* intersect P6 | P0 P1 */                                 \
            FUNC_NAME##_seg(image, payload, x0, y0, floor(r + 0.5), y, x, y);  \
            r = (x1 - x2) * (t - y2) / (y1 - y2) +                             \
                x2; /* intersect P7 | P1 P2 */                                 \
            x0 = x;                                                            \
            x1 = floor(r + 0.5);                                               \
            y0 = y1 = y; /* P0 = P6, P1 = P7 */                                \
        }                                                                      \
        FUNC_NAME##_seg(image, payload, x0, y0, x1, y1, x2,                    \
                        y2); /* remaining part */                              \
    }
BEZIER_PIXEL_ITERATOR(improvement, cprimim_compare_pixel_callback)
BEZIER_PIXEL_ITERATOR(draw, cprimim_draw_pixel_callback)

void cprimim_draw_bezier(cprimim_Image *image, cprimim_bezier *bezier,
                         cprimim_Color color) {
    cprimim_DrawData data = {0};
    data.color = &color;
    data.output = image;
    draw(image, bezier, &data);
}
static inline void clamp_bezier(cprimim_bezier *bz, int columns, int rows) {
    for (int i = 0; i < 3; i++) {
        if (bz->points[i].x < 0)
            bz->points[i].x = 0;
        else if (bz->points[i].x >= columns)
            bz->points[i].x = columns - 1;
        if (bz->points[i].y < 0)
            bz->points[i].y = 0;
        else if (bz->points[i].y >= rows)
            bz->points[i].y = rows - 1;
    }
}
BEST_FIT(bezier)
BEST_INITIAL(bezier)
SORT(bezier)

SHAPE_APPROX(bezier)


void write_bezier_svg(FILE * file, cprimim_bezier* bezier){
    fprintf(file, "<path d=\"M ");
    fprintf(file, "%i %i Q", bezier->points[0].x,bezier->points[0].y);
    for(int k=1; k<3; k++) fprintf(file, " %i %i", bezier->points[k].x,bezier->points[k].y);
    fprintf(file, "\" stroke=\"rgb(%u,%u,%u)\" fill=\"transparent\"/>",bezier->color.r,bezier->color.g,bezier->color.b);
}
