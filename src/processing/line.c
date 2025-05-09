#include "line.h"
#include "color.h"
#include "cprimim.h"
#include "image.h"
#include "image_internal.h"
#include "point.h"
#include "utils.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#define MUTATION_DISTANCE 15
#define PROCESS_LINE(CALLBACK)                                                 \
    int x0 = line.points[0].x;                                                 \
    int x1 = line.points[1].x;                                                 \
    int y0 = line.points[0].y;                                                 \
    int y1 = line.points[1].y;                                                 \
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;                              \
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;                              \
    int err = dx - dy, e2, x2, y2; /* error value e_xy */                      \
    int ed = dx + dy == 0 ? 1 : (dx * dx + dy * dy);                           \
    int r = 0;                                                                 \
    int g = 0;                                                                 \
    int b = 0;                                                                 \
    size_t index = 0;                                                          \
    size_t count = 0;                                                          \
    for (thickness = (thickness + 1) / 2;;) { /* pixel loop */                 \
        if (x0 >= 0 && y0 >= 0 && x0 < image->columns && y0 < image->rows) {   \
            assert(x0 >= 0 && y0 >= 0 && x0 < image->columns &&                \
                   y0 < image->rows);                                          \
            CALLBACK(image, x0, y0, data);                                     \
        }                                                                      \
        e2 = err;                                                              \
        x2 = x0;                                                               \
        if (2 * e2 >= -dx) { /* x step */                                      \
            for (e2 += dy, y2 = y0; e2 * e2 < ed * thickness * thickness &&    \
                                    (y1 != y2 || dx > dy);                     \
                 e2 += dx) {                                                   \
                y2 += sy;                                                      \
                if (x0 >= 0 && y2 >= 0 && x0 < image->columns &&               \
                    y2 < image->rows) {                                        \
                    assert(x0 >= 0 && y2 >= 0 && x0 < image->columns &&        \
                           y2 < image->rows);                                  \
                    CALLBACK(image, x0, y2, data);                             \
                }                                                              \
            }                                                                  \
            e2 = err;                                                          \
            err -= dy;                                                         \
            x0 += sx;                                                          \
        }                                                                      \
        if (2 * e2 <= dy) { /* y step */                                       \
            for (e2 = dx - e2; e2 * e2 < ed * thickness * thickness &&         \
                               (x1 != x2 || dx < dy);                          \
                 e2 += dy) {                                                   \
                x2 += sx;                                                      \
                if (x2 >= 0 && y0 >= 0 && x2 < image->columns &&               \
                    y0 < image->rows) {                                        \
                    assert(x2 >= 0 && y0 >= 0 && x2 < image->columns &&        \
                           y0 < image->rows);                                  \
                    CALLBACK(image, x2, y0, data);                             \
                }                                                              \
            }                                                                  \
            err += dx;                                                         \
            y0 += sy;                                                          \
        }                                                                      \
        if (x0 == x1)                                                          \
            break;                                                             \
    }

void cprimim_randomize_line(cprimim_Line *line, int columns, int rows) {
    cprimim_randomize_point(&line->points[0], columns, rows);
    do {
        cprimim_randomize_point(&line->points[1], columns, rows);
    } while (line->points[0].x == line->points[1].x &&
             line->points[0].y == line->points[1].y);
}

void cprimim_randomize_line_local(cprimim_Line *line, int columns, int rows) {
    cprimim_randomize_point(&line->points[0], columns, rows);
    line->points[1] = line->points[0];
    do {
        cprimim_mutate_point(&line->points[1], columns, rows, 30);
    } while (line->points[0].x == line->points[1].x &&
             line->points[0].y == line->points[1].y);
}
void cprimim_mutate_line(cprimim_Line *line, int columns, int rows) {
    int index = cprimim_uniform_distribution(0, 2);
    do {
        cprimim_mutate_point(&line->points[index], columns, rows,
                             MUTATION_DISTANCE);
    } while (line->points[0].x == line->points[1].x &&
             line->points[0].y == line->points[1].y);
}
void cprimim_draw_line(cprimim_Image *image, cprimim_Line line,
                       cprimim_Color color, double thickness) {
    cprimim_Color *data = &color;
    PROCESS_LINE(cprimim_draw_pixel_callback);
}
cprimim_Color cprimim_average_color_line(cprimim_Image *image,
                                         cprimim_Line line, double thickness) {
    cprimim_Color result = {0};
    cprimim_AvgColor avg = {0};
    cprimim_AvgColor *data = &avg;
    PROCESS_LINE(cprimim_average_color_callback);
    assert(avg.count > 0);
    result.r = avg.r / avg.count;
    result.g = avg.g / avg.count;
    result.b = avg.b / avg.count;
    return result;
}
int cprimim_line_improvement(cprimim_Image *image, cprimim_Image *output,
                             cprimim_Line line, cprimim_Color color,
                             double thickness) {
    int result;
    cprimim_Comparator comparator = {0};
    comparator.color = &color;
    comparator.other = output;
    cprimim_Comparator *data = &comparator;
    PROCESS_LINE(cprimim_compare_pixel_callback);
    return comparator.improvement;
}
void cprimim_line_approx(cprimim_Image *input, cprimim_Image *output,
                         int number_of_lines, int max_number_of_tries,
                         double thickness) {
    cprimim_Line line = {0};
    cprimim_randomize_line(&line, input->columns, input->rows);
    cprimim_Color color = cprimim_average_color_line(input, line, thickness);
    cprimim_Line old_line = line;
    cprimim_Image output_buffer = {0};
    cprimim_Color avg = cprimim_avg_color(input);
    cprimim_set_background(output, &avg);
    for (int k = 0; k < number_of_lines; k++) {
        int total_tries = 0;
        int current_improvement =
            cprimim_line_improvement(input, output, line, color, thickness);
        int new_improvement = current_improvement;
        int number_of_tries = 0;
        if (k % 2 == 0) {
            cprimim_randomize_line(&line, input->columns, input->rows);
        } else {
            cprimim_randomize_line_local(&line, input->columns, input->rows);
        }

        while (number_of_tries < max_number_of_tries) {
            total_tries++;

            old_line = line;

            // cprimim_set_image(output, output_buffer_pointer);
            cprimim_mutate_line(&line, input->columns, input->rows);

            color = cprimim_average_color_line(input, line, thickness);

            // cprimim_draw_line(output_buffer_pointer, line, color, thickness);
            new_improvement =
                cprimim_line_improvement(input, output, line, color, thickness);

            if (new_improvement >= current_improvement) {
                line = old_line;
                number_of_tries++;
            } else {
                current_improvement = new_improvement;
                number_of_tries = 0;
            }
        }
        cprimim_draw_line(output, line, color, thickness);
        // printf("total tries for this line: %d, line number is %d\n",
        //        total_tries, k);
        // cprimim_set_image(output_buffer_pointer, output);
    }
    return;
}
