#include "bezier.h"
#include "color.h"
#include "cprimim_internal.h"
#include "optimize.h"
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

#include <cairo/cairo.h>

#define THICKNESS_MAX 6

static void draw_bezier_cairo(CairoContext *cairo, bezier *bez)
{
    Color color = bez->color;
    if (!cairo->cr)
        return;

    cairo_set_line_width(cairo->cr, (double)bez->thickness);

    cairo_set_source_rgba(cairo->cr, u8_to_unit(color.r), u8_to_unit(color.g),
                          u8_to_unit(color.b),
                          u8_to_unit(color.alpha)); 


    cairo_move_to(cairo->cr, bez->points[0].x + 0.5, bez->points[0].y + 0.5);
    cairo_curve_to(cairo->cr,
                   bez->points[0].x + 2.0 / 3.0 * (bez->points[1].x - bez->points[0].x),
                   bez->points[0].y + 2.0 / 3.0 * (bez->points[1].y - bez->points[0].y),
                   bez->points[2].x + 2.0 / 3.0 * (bez->points[1].x - bez->points[2].x),
                   bez->points[2].y + 2.0 / 3.0 * (bez->points[1].y - bez->points[2].y),
                   bez->points[2].x, bez->points[2].y);
    cairo_stroke(cairo->cr);

    cairo_surface_flush(cairo->surf);
}
static bool not_valid_bezier(bezier *input)
{
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
static void mutate_bezier(bezier *input, int columns, int rows, int mutation_radius)
{
    uint64_t index;
    do
    {

        uint64_t random_value = fast_rand();
        index = random_value % 3;
        mutate_point(&input->points[index], columns, rows, mutation_radius);
    } while (not_valid_bezier(input));
    index = fast_rand() & 5;
    if (index)
    {
        input->thickness += uniform_distribution(-1, 2);
        input->thickness = clamp(input->thickness, 1, THICKNESS_MAX - 1);
    }
}

static bezier random_bezier(int seed_index, int n_init, int columns, int rows, int mutation_radius)
{

    int Gx = (int)floor(sqrt((double)n_init * (double)columns / (double)rows));
    if (Gx < 1)
        Gx = 1;
    int Gy = (n_init + Gx - 1) / Gx;

    int cell_x = seed_index % Gx;
    int cell_y = seed_index / Gx;

    int cell_w = columns / Gx;
    int cell_h = rows / Gy;

    if (cell_w < 1)
        cell_w = 1;
    if (cell_h < 1)
        cell_h = 1;
    bezier bz = {0};

    do
    {
        int x0 = cell_x * cell_w + uniform_distribution(0, cell_w);
        int y0 = cell_y * cell_h + uniform_distribution(0, cell_h);
        bz.points[0].x = x0;
        bz.points[0].y = y0;
        bz.points[1] = bz.points[0];
        mutate_point(&bz.points[1], columns, rows, mutation_radius);
        bz.points[2] = bz.points[0];
        mutate_point(&bz.points[2], columns, rows, mutation_radius);
    } while (not_valid_bezier(&bz));
    bz.thickness = uniform_distribution(1, THICKNESS_MAX);
    return bz;
}

// Courtesy to http://members.chello.at/%7Eeasyfilter/Bresenham.pdf
#define LINE_SEG(FUNC_NAME, CALLBACK)                                                              \
    static void FUNC_NAME(Image *image, void *payload, int x0, int y0, int x1, int y1,             \
                          int thickness)                                                           \
    {                                                                                              \
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;                                              \
        int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;                                              \
        int err = dx - dy;                                                                         \
                                                                                                   \
        int start = -(thickness / 2);                                                              \
        int end = start + thickness - 1;                                                           \
                                                                                                   \
        bool steep = (dy > dx);                                                                    \
                                                                                                   \
        while (true)                                                                               \
        {                                                                                          \
        assert(x0>=0 && x0<image->columns && y0>=0 && y0<image->rows);     /* sign of gradient must not change */           \
            if (steep)                                                                             \
            {                                                                                      \
                int lower = x0 + start < 0 ? 0 : x0 + start;                                       \
                int upper = x0 + end >= image->columns ? image->columns - 1 : x0 + end;            \
                for (int xx = lower; xx <= upper; xx++)                                            \
                {                                                                                  \
                    int yy = y0;                                                                   \
        assert(xx>=0 && xx<image->columns && yy>=0 && yy<image->rows);     /* sign of gradient must not change */           \
                    CALLBACK(image, xx, yy, payload);                                              \
                }                                                                                  \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                int lower = y0 + start < 0 ? 0 : y0 + start;                                       \
                int upper = y0 + end >= image->rows ? image->rows - 1 : y0 + end;                  \
                for (int yy = lower; yy <= upper; yy++)                                            \
                {                                                                                  \
                    int xx = x0;                                                                   \
        assert(xx>=0 && xx<image->columns && yy>=0 && yy<image->rows);     /* sign of gradient must not change */           \
                    CALLBACK(image, xx, yy, payload);                                              \
                }                                                                                  \
            }                                                                                      \
                                                                                                   \
            if (x0 == x1 && y0 == y1)                                                              \
                break;                                                                             \
                                                                                                   \
            int e2 = err * 2;                                                                      \
            if (e2 > -dy)                                                                          \
            {                                                                                      \
                err -= dy;                                                                         \
                x0 += sx;                                                                          \
            }                                                                                      \
            if (e2 < dx)                                                                           \
            {                                                                                      \
                err += dx;                                                                         \
                y0 += sy;                                                                          \
            }                                                                                      \
        }                                                                                          \
        return;                                                                                    \
    }
#define BEZIER_SEG(FUNC_NAME, CALLBACK)                                                            \
    static void FUNC_NAME(Image *image, void *payload, int x0, int y0, int x1, int y1, int x2,     \
                          int y2, int thickness)                                                   \
    { /* plot a limited quadratic bezier segment */                                                \
        int sx = x2 - x1, sy = y2 - y1;                                                            \
        long xx = x0 - x1, yy = y0 - y1, xy;      /* relative values for checks */                 \
        int dx, dy, err, cur = xx * sy - yy * sx; /* curvature */                                  \
        assert(xx * sx <= 0 && yy * sy <= 0);     /* sign of gradient must not change */           \
        if (sx * (long)sx + sy * (long)sy > xx * xx + yy * yy)                                     \
        { /* begin with longer part */                                                             \
            x2 = x0;                                                                               \
            x0 = sx + x1;                                                                          \
            y2 = y0;                                                                               \
            y0 = sy + y1;                                                                          \
            cur = -cur; /* swap P0 P2 */                                                           \
        }                                                                                          \
        int start = -(thickness / 2);                                                              \
        int end = start + thickness - 1;                                                           \
                                                                                                   \
        int last_x = x0, last_y = y0;                                                              \
        int step_dx = 0, step_dy = 0;                                                              \
        if (abs(x2 - x0) >= abs(y2 - y0))                                                          \
            step_dx = (x0 < x2) ? 1 : -1;                                                          \
        else                                                                                       \
            step_dy = (y0 < y2) ? 1 : -1;                                                          \
        if (cur != 0)                                                                              \
        { /* no straight line */                                                                   \
            xx += sx;                                                                              \
            xx *= sx = x0 < x2 ? 1 : -1; /* x step direction */                                    \
            yy += sy;                                                                              \
            yy *= sy = y0 < y2 ? 1 : -1; /* y step direction */                                    \
            xy = 2 * xx * yy;                                                                      \
            xx *= xx;                                                                              \
            yy *= yy; /* differences 2nd degree */                                                 \
            if (cur * sx * sy < 0)                                                                 \
            { /* negated curvature? */                                                             \
                xx = -xx;                                                                          \
                yy = -yy;                                                                          \
                xy = -xy;                                                                          \
                cur = -cur;                                                                        \
            }                                                                                      \
            dx = 4 * sy * cur * (x1 - x0) + xx - xy; /* differences 1st degree */                  \
            dy = 4 * sx * cur * (y0 - y1) + yy - xy;                                               \
            xx += xx;                                                                              \
            yy += yy;                                                                              \
            err = dx + dy + xy; /* error 1st step */                                               \
            do                                                                                     \
            {                                                                                      \
                /* stamp thickness along local normal n = (-step_dy, step_dx) */                   \
                int nx = -step_dy;                                                                 \
                int ny = step_dx;                                                                  \
                if (nx == 0 && ny == 0)                                                            \
                {                                                                                  \
                    if ((unsigned)x0 < (unsigned)image->columns &&                                 \
                        (unsigned)y0 < (unsigned)image->rows)                                      \
                        CALLBACK(image, x0, y0, payload);                                          \
                }                                                                                  \
                else                                                                               \
                {                                                                                  \
                    for (int k = start; k <= end; k++)                                             \
                    {                                                                              \
                        int px = x0 + k * nx;                                                      \
                        int py = y0 + k * ny;                                                      \
                        if ((unsigned)px < (unsigned)image->columns &&                             \
                            (unsigned)py < (unsigned)image->rows)                                  \
                            CALLBACK(image, px, py, payload);                                      \
                    }                                                                              \
                }                                                                                  \
                if (x0 == x2 && y0 == y2)                                                          \
                    return;        /* last pixel -> curve finished */                              \
                y1 = 2 * err < dx; /* save value for test of y step */                             \
                if (2 * err > dy)                                                                  \
                {                                                                                  \
                    x0 += sx;                                                                      \
                    dx -= xy;                                                                      \
                    err += dy += yy;                                                               \
                } /* x step */                                                                     \
                if (y1)                                                                            \
                {                                                                                  \
                    y0 += sy;                                                                      \
                    dy -= xy;                                                                      \
                    err += dx += xx;                                                               \
                } /* y step */                                                                     \
                                                                                                   \
                /* update local step direction based on actual move */                             \
                int mdx = x0 - last_x;                                                             \
                int mdy = y0 - last_y;                                                             \
                if (mdx || mdy)                                                                    \
                {                                                                                  \
                    step_dx = (mdx > 0) - (mdx < 0);                                               \
                    step_dy = (mdy > 0) - (mdy < 0);                                               \
                    last_x = x0;                                                                   \
                    last_y = y0;                                                                   \
                }                                                                                  \
            } while (dy < 0 && dx > 0); /* gradient negates -> algorithm fails */                  \
        }                                                                                          \
        FUNC_NAME##_line(image, payload, x0, y0, x2, y2,                                           \
                         thickness); /* plot remaining part to end */                              \
    }
#define BEZIER_PIXEL_ITERATOR(FUNC_NAME, CALLBACK)                                                 \
    LINE_SEG(FUNC_NAME##_seg_line, CALLBACK)                                                       \
    BEZIER_SEG(FUNC_NAME##_seg, CALLBACK)                                                          \
    static void FUNC_NAME(Image *image, bezier *bezier, void *payload)                             \
    { /* plot any quadratic bezier curve */                                                        \
        int x0 = bezier->points[0].x;                                                              \
        int x1 = bezier->points[1].x;                                                              \
        int x2 = bezier->points[2].x;                                                              \
        int y0 = bezier->points[0].y;                                                              \
        int y1 = bezier->points[1].y;                                                              \
        int y2 = bezier->points[2].y;                                                              \
        int x = x0 - x1, y = y0 - y1;                                                              \
        float t = x0 - 2 * x1 + x2, r;                                                             \
        if ((long)x * (x2 - x1) > 0)                                                               \
        {                                /* horizontal cut at P4? */                               \
            if ((long)y * (y2 - y1) > 0) /* vertical cut at P6 too? */                             \
                if (fabs((y0 - 2 * y1 + y2) / t * x) > abs(y))                                     \
                { /* which first? */                                                               \
                    x0 = x2;                                                                       \
                    x2 = x + x1;                                                                   \
                    y0 = y2;                                                                       \
                    y2 = y + y1; /* swap points */                                                 \
                } /* now horizontal cut at P4 comes first */                                       \
            t = (x0 - x1) / t;                                                                     \
            r = (1 - t) * ((1 - t) * y0 + 2.0 * t * y1) + t * t * y2; /* By(t=P4) */               \
            t = (x0 * x2 - x1 * x1) * t / (x0 - x1);                  /* gradient dP4/dx=0 */      \
            x = floor(t + 0.5);                                                                    \
            y = floor(r + 0.5);                                                                    \
            r = (y1 - y0) * (t - x0) / (x1 - x0) + y0; /* intersect P3 | P0 P1 */                  \
            FUNC_NAME##_seg(image, payload, x0, y0, x, floor(r + 0.5), x, y, bezier->thickness);   \
            r = (y1 - y2) * (t - x2) / (x1 - x2) + y2; /* intersect P4 | P1 P2 */                  \
            x0 = x1 = x;                                                                           \
            y0 = y;                                                                                \
            y1 = floor(r + 0.5); /* P0 = P4, P1 = P8 */                                            \
        }                                                                                          \
        if ((long)(y0 - y1) * (y2 - y1) > 0)                                                       \
        { /* vertical cut at P6? */                                                                \
            t = y0 - 2 * y1 + y2;                                                                  \
            t = (y0 - y1) / t;                                                                     \
            r = (1 - t) * ((1 - t) * x0 + 2.0 * t * x1) + t * t * x2; /* Bx(t=P6) */               \
            t = (y0 * y2 - y1 * y1) * t / (y0 - y1);                  /* gradient dP6/dy=0 */      \
            x = floor(r + 0.5);                                                                    \
            y = floor(t + 0.5);                                                                    \
            r = (x1 - x0) * (t - y0) / (y1 - y0) + x0; /* intersect P6 | P0 P1 */                  \
            FUNC_NAME##_seg(image, payload, x0, y0, floor(r + 0.5), y, x, y, bezier->thickness);   \
            r = (x1 - x2) * (t - y2) / (y1 - y2) + x2; /* intersect P7 | P1 P2 */                  \
            x0 = x;                                                                                \
            x1 = floor(r + 0.5);                                                                   \
            y0 = y1 = y; /* P0 = P6, P1 = P7 */                                                    \
        }                                                                                          \
        FUNC_NAME##_seg(image, payload, x0, y0, x1, y1, x2, y2,                                    \
                        bezier->thickness); /* remaining part */                                   \
    }
BEZIER_PIXEL_ITERATOR(improvement, compare_pixel_callback)
BEZIER_PIXEL_ITERATOR(draw, draw_pixel_callback)

static void draw_bezier(CairoContext* cairo, bezier *bezier)
{
    // DrawData data = {0};
    // data.color = &color;
    // data.output = image;
    // draw(image, bezier, &data);
    draw_bezier_cairo(cairo, bezier);
}
static inline void clamp_bezier(bezier *bz, int columns, int rows)
{
    for (int i = 0; i < 3; i++)
    {
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
#define COARSE_STRIDE 2
SHAPE_APPROX(bezier)

void cprimim_write_bezier_svg(FILE *file, bezier *bezier, double alpha)
{
    fprintf(file, "<path d=\"M ");
    fprintf(file, "%i %i Q", bezier->points[0].x, bezier->points[0].y);
    for (int k = 1; k < 3; k++)
        fprintf(file, " %i %i", bezier->points[k].x, bezier->points[k].y);
    fprintf(file,
            "\" stroke=\"rgb(%u,%u,%u)\" stroke-opacity=\"%f\" fill=\"transparent\" "
            "stroke-linecap=\"round\" stroke-width=\"%d\"/>",
            bezier->color.r, bezier->color.g, bezier->color.b, alpha, bezier->thickness);
}
