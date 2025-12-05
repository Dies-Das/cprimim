#include "bezier.h"
#include "color.h"
#include "cprimim.h"
#include "image.h"
#include "image_internal.h"
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
#include <string.h>
#include <time.h>

#define SIMPLEX_SIZE 7
void cprimim_best_fit(cprimim_Image *image, cprimim_Image *output,
                      cprimim_Bezier *bezier);
int compar(const void *a, const void *b) {
    const cprimim_Bezier *left = a;
    const cprimim_Bezier *right = b;
    if (left->improvement > right->improvement) {
        return 1;
    }
    return -1;
}
bool not_valid_bezier(cprimim_Bezier *input) {
    bool valid = 0;
    valid |= (input->points[0].x == input->points[1].x &&
              input->points[0].y == input->points[1].y);
    valid |= (input->points[2].x == input->points[1].x &&
              input->points[2].y == input->points[1].y);
    valid |= (input->points[0].x == input->points[2].x &&
              input->points[0].y == input->points[2].y);
    return valid;
}
bool not_valid_initial(cprimim_Bezier *input) {
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
void mutate_bezier(cprimim_Bezier *input, int columns, int rows) {
    do {

        uint64_t random_value = fast_rand();
        uint64_t index = random_value % 3;
        cprimim_mutate_point(&input->points[index], columns, rows,
                             MUTATION_DISTANCE);
    } while (not_valid_initial(input));
}

void random_bezier(cprimim_Bezier *input, int columns, int rows) {
    cprimim_randomize_point(&input->points[0], columns, rows);
    input->points[1] = input->points[0];
    cprimim_mutate_point_uniform(&input->points[1], columns, rows,
                                 MUTATION_DISTANCE);
    input->points[2] = input->points[1];
    cprimim_mutate_point_uniform(&input->points[2], columns, rows,
                                 MUTATION_DISTANCE);
    mutate_bezier(input, columns, rows);
}
static cprimim_Bezier stratified_random_bezier(int seed_index, int n_init,
                                               int columns, int rows) {
    int G = (int)ceil(sqrt((double)n_init));
    int cell_x = seed_index % G;
    int cell_y = seed_index / G;
    int cell_w = columns / G;
    int cell_h = rows / G;

    cprimim_Bezier bz = {0};

    do {
        int x0 = cell_x * cell_w + cprimim_uniform_distribution(0, cell_w);
        int y0 = cell_y * cell_h + cprimim_uniform_distribution(0, cell_h);
        bz.points[0].x = x0;
        bz.points[0].y = y0;
        bz.points[1] = bz.points[0];
        cprimim_mutate_point_uniform(&bz.points[1], columns, rows,
                                     MUTATION_DISTANCE);
        bz.points[2] = bz.points[1];
        cprimim_mutate_point_uniform(&bz.points[2], columns, rows,
                                     MUTATION_DISTANCE);
    } while (not_valid_initial(&bz));
    // mutate_bezier(&bz, columns, rows);
    return bz;
}

static cprimim_Bezier pick_best_initial_bezier(cprimim_Image *orig,
                                               cprimim_Image *curr, int columns,
                                               int rows, int n_init) {
    cprimim_Bezier best = {.improvement = INT_MAX};

    for (int i = 0; i < n_init; i++) {
        cprimim_Bezier cand =
            stratified_random_bezier(i, n_init, columns, rows);
        cprimim_best_fit(orig, curr, &cand);
        if (cand.improvement < best.improvement) {
            best = cand;
        }
    }
    return best;
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
        int x2, int y2) { /* plot a limited quadratic Bezier segment */        \
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
        cprimim_Image *image, cprimim_Bezier *bezier,                          \
        void *payload) { /* plot any quadratic Bezier curve */                 \
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

void cprimim_draw_bezier(cprimim_Image *image, cprimim_Bezier *bezier,
                         cprimim_Color color) {
    cprimim_DrawData data = {0};
    data.color = &color;
    data.output = image;
    draw(image, bezier, &data);
}
void cprimim_best_fit(cprimim_Image *image, cprimim_Image *output,
                      cprimim_Bezier *bezier) {
    cprimim_Comparator comparator = {0};
    comparator.other = output;
    improvement(image, bezier, &comparator);
    int64_t denom = A * comparator.counter;
    bezier->color.r = cprimim_clamp(-comparator.sum_diffs[0] / denom, 0, 255);
    bezier->color.g = cprimim_clamp(-comparator.sum_diffs[1] / denom, 0, 255);
    bezier->color.b = cprimim_clamp(-comparator.sum_diffs[2] / denom, 0, 255);
    int64_t error_new =
        comparator.sum_diffs_squared[0] -
        comparator.sum_diffs[0] * comparator.sum_diffs[0] / comparator.counter;
    error_new += comparator.sum_diffs_squared[1] - comparator.sum_diffs[1] *
                                                       comparator.sum_diffs[1] /
                                                       comparator.counter;
    error_new += comparator.sum_diffs_squared[2] - comparator.sum_diffs[2] *
                                                       comparator.sum_diffs[2] /
                                                       comparator.counter;
    bezier->improvement = -(comparator.error_old - error_new / (255 * 255));
}
static inline void clamp_bezier(cprimim_Bezier *bz, int columns, int rows) {
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
static void reflect_bezier(const cprimim_Bezier *worst, const int *centroid,
                           int factor, cprimim_Bezier *out) {
    // copy the structure as a base
    *out = *worst;

    // extract worst coords and apply reflection in each dimension
    // dims: 0→p0.x, 1→p0.y, 2→p1.x, 3→p1.y, 4→p2.x, 5→p2.y
    int worst_coords[6] = {worst->points[0].x, worst->points[0].y,
                           worst->points[1].x, worst->points[1].y,
                           worst->points[2].x, worst->points[2].y};
    int *outs[6] = {&out->points[0].x, &out->points[0].y, &out->points[1].x,
                    &out->points[1].y, &out->points[2].x, &out->points[2].y};

    // 4) Loop over the 6 dimensions
    for (int d = 0; d < 6; d++) {
        int v = centroid[d] + factor * (centroid[d] - worst_coords[d]);
        *outs[d] = v;
    }
}
static void sort_bezier(cprimim_Bezier *beziers, int n) {
    for (int i = 1; i < n; i++) {
        cprimim_Bezier key = beziers[i];
        int j = i - 1;
        while (j >= 0 && beziers[j].improvement > key.improvement) {
            beziers[j + 1] = beziers[j];
            j--;
        }
        beziers[j + 1] = key;
    }
}
void cprimim_bezier_approx(cprimim_Context *context) {
    cprimim_Image *input = &context->input;
    cprimim_Image *output = &context->output;
    size_t number_of_lines = context->nr_shapes;
    size_t max_number_of_tries = context->attempts;
    size_t initial_shapes = context->initial_shapes;
    cprimim_Bezier *shapes = context->shapes;
    cprimim_Bezier *candidate_shapes = context->candidate_shapes;
    int columns = input->columns;

    int rows = input->rows;
    cprimim_Color avg = cprimim_avg_color(&context->input);
    cprimim_set_background(output, &avg);
    int global_tries = 0;
    printf("starting iteration!\n");
    uint64_t avg_iterations = 0;
    // #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        utils_srand(time(NULL) ^ (uint64_t)tid * 0x9E3779B97F4A7C15ULL);
        for (int k = 0; k < number_of_lines; k++) {
            // #pragma omp for schedule(static)
            for (int candidate = 0; candidate < context->candidates;
                 candidate++) {
                uint64_t current_iterations = 0;
                cprimim_Bezier old_candidate_shape = {0};
                cprimim_Bezier candidate_shape = {0};

                candidate_shape.improvement = INT_MAX;
                old_candidate_shape.improvement = INT_MAX;
                candidate_shape = pick_best_initial_bezier(
                    input, output, columns, rows, initial_shapes);
                size_t number_of_tries = 0;
                while (number_of_tries < max_number_of_tries) {
                    current_iterations++;
                    mutate_bezier(&candidate_shape, input->columns,
                                  input->rows);
                    cprimim_best_fit(input, output, &candidate_shape);
                    if (candidate_shape.improvement >=
                        old_candidate_shape.improvement) {
                        candidate_shape = old_candidate_shape;
                        number_of_tries++;

                    } else {
                        old_candidate_shape = candidate_shape;
                        number_of_tries = 0;
                    }
                }

                candidate_shapes[candidate] = candidate_shape;
            }
            {
                sort_bezier(candidate_shapes, context->candidates);
                shapes[k] = candidate_shapes[0];
                cprimim_draw_bezier(output, &shapes[k], shapes[k].color);
            }
        }
    }
    printf("done! with average iterations %lu\n", avg_iterations);
    return;
}
