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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void mutate_bezier(cprimim_Bezier *input, int columns, int rows) {
    uint64_t random_value = fast_rand();
    uint64_t index = random_value & 1 + random_value & 2;
    cprimim_mutate_point(&input->points[index], columns, rows,
                         MUTATION_DISTANCE);
}

void random_bezier(cprimim_Bezier *input, int columns, int rows) {
    cprimim_randomize_point(&input->points[0], columns, rows);
    input->points[1] = input->points[0];
    cprimim_mutate_point(&input->points[1], columns, rows, MUTATION_DISTANCE);
    input->points[2] = input->points[1];
    cprimim_mutate_point(&input->points[2], columns, rows, MUTATION_DISTANCE);
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
// Courtesy to http://members.chello.at/%7Eeasyfilter/Bresenham.pdf
void plot_line(cprimim_Image *image, cprimim_AvgColor *avg,
               cprimim_IndexBuffer *buffer, int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */
    uint64_t index = 0;
    for (;;) { /* loop */

        cprimim_average_color_callback(image, x0, y0, avg);
        index = CHANNELS * (image->columns * y0 + x0);
        buffer->indices[buffer->count] = index;
        buffer->count++;
        e2 = 2 * err;
        if (e2 >= dy) { /* e_xy+e_x > 0 */
            if (x0 == x1)
                break;
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) { /* e_xy+e_y < 0 */
            if (y0 == y1)
                break;
            err += dx;
            y0 += sy;
        }
    }
}
void plot_quad_bezier_seg(
    cprimim_Image *image, cprimim_AvgColor *avg, cprimim_IndexBuffer *buffer,
    int x0, int y0, int x1, int y1, int x2,
    int y2) { /* plot a limited quadratic Bezier segment */
    int sx = x2 - x1, sy = y2 - y1;
    long xx = x0 - x1, yy = y0 - y1, xy; /* relative values for checks */
    double dx, dy, err, cur = xx * sy - yy * sx; /* curvature */
    uint64_t index = 0;
    assert(xx * sx <= 0 && yy * sy <= 0); /* sign of gradient must not change */
    if (sx * (long)sx + sy * (long)sy >
        xx * xx + yy * yy) { /* begin with longer part */
        x2 = x0;
        x0 = sx + x1;
        y2 = y0;
        y0 = sy + y1;
        cur = -cur; /* swap P0 P2 */
    }
    if (cur != 0) { /* no straight line */
        xx += sx;
        xx *= sx = x0 < x2 ? 1 : -1; /* x step direction */
        yy += sy;
        yy *= sy = y0 < y2 ? 1 : -1; /* y step direction */
        xy = 2 * xx * yy;
        xx *= xx;
        yy *= yy;                /* differences 2nd degree */
        if (cur * sx * sy < 0) { /* negated curvature? */
            xx = -xx;
            yy = -yy;
            xy = -xy;
            cur = -cur;
        }
        dx = 4.0 * sy * cur * (x1 - x0) + xx - xy; /* differences 1st degree */
        dy = 4.0 * sx * cur * (y0 - y1) + yy - xy;
        xx += xx;
        yy += yy;
        err = dx + dy + xy; /* error 1st step */
        do {
            cprimim_average_color_callback(image, x0, y0, avg);
            index = CHANNELS * (image->columns * y0 + x0);
            buffer->indices[buffer->count] = index;
            buffer->count++;
            if (x0 == x2 && y0 == y2)
                return;        /* last pixel -> curve finished */
            y1 = 2 * err < dx; /* save value for test of y step */
            if (2 * err > dy) {
                x0 += sx;
                dx -= xy;
                err += dy += yy;
            } /* x step */
            if (y1) {
                y0 += sy;
                dy -= xy;
                err += dx += xx;
            } /* y step */
        } while (dy < 0 && dx > 0); /* gradient negates -> algorithm fails */
    }
    plot_line(image, avg, buffer, x0, y0, x2,
              y2); /* plot remaining part to end */
}
cprimim_AvgColor bezier_buffer_and_color(
    cprimim_Image *image, cprimim_Bezier *bezier,
    cprimim_IndexBuffer *buffer) { /* plot any quadratic Bezier curve */
    int x0 = bezier->points[0].x;
    int x1 = bezier->points[1].x;
    int x2 = bezier->points[2].x;
    int y0 = bezier->points[0].y;
    int y1 = bezier->points[1].y;
    int y2 = bezier->points[2].y;
    int x = x0 - x1, y = y0 - y1;
    double t = x0 - 2 * x1 + x2, r;
    cprimim_AvgColor avg = {0};
    buffer->count = 0;
    if ((long)x * (x2 - x1) > 0) {   /* horizontal cut at P4? */
        if ((long)y * (y2 - y1) > 0) /* vertical cut at P6 too? */
            if (fabs((y0 - 2 * y1 + y2) / t * x) > abs(y)) { /* which first? */
                x0 = x2;
                x2 = x + x1;
                y0 = y2;
                y2 = y + y1; /* swap points */
            } /* now horizontal cut at P4 comes first */
        t = (x0 - x1) / t;
        r = (1 - t) * ((1 - t) * y0 + 2.0 * t * y1) + t * t * y2; /* By(t=P4) */
        t = (x0 * x2 - x1 * x1) * t / (x0 - x1); /* gradient dP4/dx=0 */
        x = floor(t + 0.5);
        y = floor(r + 0.5);
        r = (y1 - y0) * (t - x0) / (x1 - x0) + y0; /* intersect P3 | P0 P1 */
        plot_quad_bezier_seg(image, &avg, buffer, x0, y0, x, floor(r + 0.5), x,
                             y);
        r = (y1 - y2) * (t - x2) / (x1 - x2) + y2; /* intersect P4 | P1 P2 */
        x0 = x1 = x;
        y0 = y;
        y1 = floor(r + 0.5); /* P0 = P4, P1 = P8 */
    }
    if ((long)(y0 - y1) * (y2 - y1) > 0) { /* vertical cut at P6? */
        t = y0 - 2 * y1 + y2;
        t = (y0 - y1) / t;
        r = (1 - t) * ((1 - t) * x0 + 2.0 * t * x1) + t * t * x2; /* Bx(t=P6) */
        t = (y0 * y2 - y1 * y1) * t / (y0 - y1); /* gradient dP6/dy=0 */
        x = floor(r + 0.5);
        y = floor(t + 0.5);
        r = (x1 - x0) * (t - y0) / (y1 - y0) + x0; /* intersect P6 | P0 P1 */
        plot_quad_bezier_seg(image, &avg, buffer, x0, y0, floor(r + 0.5), y, x,
                             y);
        r = (x1 - x2) * (t - y2) / (y1 - y2) + x2; /* intersect P7 | P1 P2 */
        x0 = x;
        x1 = floor(r + 0.5);
        y0 = y1 = y; /* P0 = P6, P1 = P7 */
    }
    plot_quad_bezier_seg(image, &avg, buffer, x0, y0, x1, y1, x2,
                         y2); /* remaining part */
    return avg;
}

void cprimim_draw_bezier(cprimim_Image *image, cprimim_IndexBuffer *buffer,
                         cprimim_Color color) {
    cprimim_Color *data = &color;
    for (uint64_t k = 0; k < buffer->count; k++) {
        cprimim_draw_pixel_callback(image, buffer->indices[k], data);
    }
}
cprimim_Color cprimim_average_color_and_buffer_bezier(
    cprimim_Image *image, cprimim_IndexBuffer *buffer, cprimim_Bezier bezier) {
    cprimim_Color result = {0};
    // cprimim_AvgColor avg = {0};
    // cprimim_AvgColor *data = &avg;
    assert(bezier.points[0].x != bezier.points[1].x ||
           bezier.points[0].y != bezier.points[1].y);
    cprimim_AvgColor avg = bezier_buffer_and_color(image, &bezier, buffer);
    assert(avg.count > 0);
    result.r = avg.r / avg.count;
    result.g = avg.g / avg.count;
    result.b = avg.b / avg.count;
    return result;
}
int cprimim_bezier_improvement(cprimim_Image *image, cprimim_Image *output,
                               cprimim_IndexBuffer *buffer,
                               cprimim_Bezier bezier, cprimim_Color color) {
    int result;
    cprimim_Comparator comparator = {0};
    comparator.color = &color;
    comparator.other = output;
    for (uint64_t k = 0; k < buffer->count; k++) {
        cprimim_compare_pixel_callback(image, output, buffer->indices[k],
                                       &comparator);
    }

    return comparator.improvement;
}
#if 0
void cprimim_bezier_approx(cprimim_Image *input, cprimim_Image *output,
                           int number_of_lines, int max_number_of_tries) {
    cprimim_Bezier bezier = {0};
    utils_srand(time(NULL));
    int columns = input->columns;
    int rows = input->rows;
    random_bezier(&bezier, columns, rows);
    while (not_valid_bezier(&bezier)) {
        random_bezier(&bezier, columns, rows);
    }
    cprimim_Bezier old_bezier = {0};
    cprimim_Image output_buffer = {0};
    cprimim_Color avg = cprimim_avg_color(input);
    cprimim_set_background(output, &avg);
    cprimim_IndexBuffer buffer = {0};
    cprimim_IndexBuffer best_buffer = {0};
    buffer.indices = malloc(sizeof(uint64_t) *
                            (uint64_t)(sqrt(input->columns * input->columns +
                                            input->rows * input->rows) *
                                       2));
    best_buffer.indices = malloc(
        sizeof(uint64_t) * (uint64_t)(sqrt(input->columns * input->columns +
                                           input->rows * input->rows) *
                                      2));
    cprimim_Color color =
        cprimim_average_color_and_buffer_bezier(input, &buffer, bezier);
    cprimim_Color best_color = color;
    int global_tries = 0;
    printf("starting iteration!\n");
    for (int k = 0; k < number_of_lines; k++) {
        int total_tries = 0;
        int current_improvement =
            cprimim_bezier_improvement(input, output, &buffer, bezier, color);
        int new_improvement = current_improvement;
        int number_of_tries = 0;

        random_bezier(&bezier, input->columns, input->rows);
        while (not_valid_initial(&bezier)) {
            random_bezier(&bezier, columns, rows);
        }
        old_bezier = bezier;
        while (number_of_tries < max_number_of_tries) {

            total_tries++;

            // cprimim_set_image(output, output_buffer_pointer);
            mutate_bezier(&bezier, input->columns, input->rows);
            if (not_valid_initial(&bezier)) {

                bezier = old_bezier;
                continue;
            }
            color =
                cprimim_average_color_and_buffer_bezier(input, &buffer, bezier);
            // cprimim_draw_bezier(output_buffer_pointer, bezier, color, );
            new_improvement = cprimim_bezier_improvement(input, output, &buffer,
                                                         bezier, color);
            bezier.improvement = new_improvement;
            if (new_improvement >= current_improvement) {
                bezier = old_bezier;
                number_of_tries++;

            } else {
                current_improvement = new_improvement;
                number_of_tries = 0;
                memcpy(best_buffer.indices, buffer.indices,
                       sizeof(uint64_t) * buffer.count);
                best_buffer.count = buffer.count;
                best_color = color;
            }
        }
        global_tries += total_tries;
        cprimim_draw_bezier(output, &best_buffer, best_color);
        // cprimim_set_image(output_buffer_pointer, output);
    }
    printf("done!\n");
    free(buffer.indices);
    printf("tries overall: %d\n", global_tries);
    return;
}
#else
void cprimim_bezier_approx(cprimim_Context *context) {
    cprimim_Image *input = &context->input;
    cprimim_Image *output = &context->output;
    cprimim_Bezier *candidates = context->candidate_shapes;
    size_t number_of_lines = context->nr_shapes;
    size_t max_number_of_tries = context->attempts;
    int columns = input->columns;

    int rows = input->rows;
    cprimim_Color avg = cprimim_avg_color(&context->input);
    cprimim_set_background(output, &avg);
    int global_tries = 0;
    printf("starting iteration!\n");
#pragma omp parallel
    {
        for (int k = 0; k < number_of_lines; k++) {
#pragma omp for schedule(static)
            for (int candidate = 0; candidate < context->candidates;
                 candidate++) {
                cprimim_IndexBuffer *buffer =
                    &context->working_buffer[candidate];
                cprimim_IndexBuffer *best_buffer =
                    &context->result_buffer[candidate];
                cprimim_Color color = {0};
                cprimim_Color best_color = {0};

                size_t number_of_tries = 0;
                int current_improvement = INT_MAX;
                int new_improvement = 0;
                cprimim_Bezier *candidate_shape = &candidates[candidate];
                do {
                    random_bezier(candidate_shape, columns, rows);
                } while (not_valid_initial(candidate_shape));
                cprimim_Bezier old_candidate_shape = *candidate_shape;
                while (number_of_tries < max_number_of_tries) {

                    // cprimim_set_image(output, output_buffer_pointer);
                    mutate_bezier(candidate_shape, input->columns, input->rows);
                    if (not_valid_initial(candidate_shape)) {

                        *candidate_shape = old_candidate_shape;
                        continue;
                    }
                    color = cprimim_average_color_and_buffer_bezier(
                        input, buffer, *candidate_shape);
                    // cprimim_draw_bezier(output_buffer_pointer, bezier, color,
                    // );
                    new_improvement = cprimim_bezier_improvement(
                        input, output, buffer, *candidate_shape, color);
                    candidate_shape->improvement = new_improvement;
                    if (new_improvement >= current_improvement) {
                        *candidate_shape = old_candidate_shape;
                        number_of_tries++;

                    } else {
                        current_improvement = new_improvement;
                        number_of_tries = 0;
                        memcpy(best_buffer->indices, buffer->indices,
                               sizeof(uint64_t) * buffer->count);
                        best_buffer->count = buffer->count;
                        best_color = color;
                    }
                }
                candidate_shape->color = best_color;
                candidate_shape->improvement = current_improvement;
            }
#pragma omp single
            {
                cprimim_draw_bezier(output, &context->result_buffer[0],
                                    candidates[0].color);
                // cprimim_set_image(output_buffer_pointer, output);
            }
        }
    }
    printf("done!\n");
    return;
}
#endif
