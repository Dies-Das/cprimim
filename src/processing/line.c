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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
cprimim_AvgColor fill_line_buffer(cprimim_Image *image, cprimim_Line line,
                                  int thickness, cprimim_IndexBuffer *buffer) {
    int x0 = line.points[0].x;
    int x1 = line.points[1].x;
    int y0 = line.points[0].y;
    int y1 = line.points[1].y;
    cprimim_AvgColor result = {0};
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx - dy, e2, x2, y2; /* error value e_xy */
    int ed = dx + dy == 0 ? 1 : (dx * dx + dy * dy);
    size_t index = 0;
    buffer->count = 0;
    for (thickness = (thickness + 1) / 2;;) { /* pixel loop */
        if (x0 >= 0 && y0 >= 0 && x0 < image->columns && y0 < image->rows) {
            assert(x0 >= 0 && y0 >= 0 && x0 < image->columns &&
                   y0 < image->rows);
            index = CHANNELS * (image->columns * y0 + x0);
            buffer->indices[buffer->count] = index;
            buffer->count++;
            cprimim_average_color_callback(image, x0, y0, &result);
        }
        e2 = err;
        x2 = x0;
        if (2 * e2 >= -dx) { /* x step */
            for (e2 += dy, y2 = y0;
                 e2 * e2 < ed * thickness * thickness && (y1 != y2 || dx > dy);
                 e2 += dx) {
                y2 += sy;
                if (x0 >= 0 && y2 >= 0 && x0 < image->columns &&
                    y2 < image->rows) {
                    assert(x0 >= 0 && y2 >= 0 && x0 < image->columns &&
                           y2 < image->rows);
                    index = CHANNELS * (image->columns * y2 + x0);
                    buffer->indices[buffer->count] = index;
                    buffer->count++;
                    cprimim_average_color_callback(image, x0, y2, &result);
                }
            }
            e2 = err;
            err -= dy;
            x0 += sx;
        }
        if (2 * e2 <= dy) { /* y step */
            for (e2 = dx - e2;
                 e2 * e2 < ed * thickness * thickness && (x1 != x2 || dx < dy);
                 e2 += dy) {
                x2 += sx;
                if (x2 >= 0 && y0 >= 0 && x2 < image->columns &&
                    y0 < image->rows) {
                    assert(x2 >= 0 && y0 >= 0 && x2 < image->columns &&
                           y0 < image->rows);
                    index = CHANNELS * (image->columns * y0 + x2);
                    buffer->indices[buffer->count] = index;
                    buffer->count++;
                    cprimim_average_color_callback(image, x2, y0, &result);
                }
            }
            err += dx;
            y0 += sy;
        }
        if (x0 == x1)
            break;
    }
    return result;
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
    int index = fast_rand() & 1;
    cprimim_mutate_point(&line->points[index], columns, rows,
                         MUTATION_DISTANCE);
}
void cprimim_draw_line(cprimim_Image *image, cprimim_IndexBuffer *buffer,
                       cprimim_Line line, cprimim_Color color,
                       double thickness) {
    cprimim_Color *data = &color;
    for (uint64_t k = 0; k < buffer->count; k++) {
        cprimim_draw_pixel_callback(image, buffer->indices[k], data);
    }
}
cprimim_Color cprimim_average_color_and_buffer_line(cprimim_Image *image,
                                                    cprimim_IndexBuffer *buffer,
                                                    cprimim_Line line,
                                                    double thickness) {
    cprimim_Color result = {0};
    // cprimim_AvgColor avg = {0};
    // cprimim_AvgColor *data = &avg;
    assert(line.points[0].x != line.points[1].x ||
           line.points[0].y != line.points[1].y);
    cprimim_AvgColor avg = fill_line_buffer(image, line, thickness, buffer);
    assert(avg.count > 0);
    result.r = avg.r / avg.count;
    result.g = avg.g / avg.count;
    result.b = avg.b / avg.count;
    return result;
}
int cprimim_line_improvement(cprimim_Image *image, cprimim_Image *output,
                             cprimim_IndexBuffer *buffer, cprimim_Line line,
                             cprimim_Color color, double thickness) {
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
void cprimim_line_approx(cprimim_Image *input, cprimim_Image *output,
                         int number_of_lines, int max_number_of_tries,
                         double thickness) {
    cprimim_Line line = {0};
    utils_srand(time(NULL));
    utils_srand(0);
    cprimim_randomize_line(&line, input->columns, input->rows);
    cprimim_Line old_line = line;
    cprimim_Image output_buffer = {0};
    cprimim_Color avg = cprimim_avg_color(input);
    cprimim_set_background(output, &avg);
    cprimim_IndexBuffer buffer = {0};
    buffer.indices = malloc(sizeof(uint64_t) *
                            (uint64_t)(sqrt(input->columns * input->columns +
                                            input->rows * input->rows) *
                                       thickness));
    cprimim_Color color =
        cprimim_average_color_and_buffer_line(input, &buffer, line, thickness);
    int global_tries = 0;
    for (int k = 0; k < number_of_lines; k++) {
        int total_tries = 0;
        int current_improvement = cprimim_line_improvement(
            input, output, &buffer, line, color, thickness);
        int new_improvement = current_improvement;
        int number_of_tries = 0;

        cprimim_randomize_line_local(&line, input->columns, input->rows);
        old_line = line;
        while (number_of_tries < max_number_of_tries) {

            total_tries++;

            // cprimim_set_image(output, output_buffer_pointer);
            cprimim_mutate_line(&line, input->columns, input->rows);
            if (line.points[0].x == line.points[1].x &&
                line.points[0].y == line.points[1].y) {

                line = old_line;
                continue;
            }
            color = cprimim_average_color_and_buffer_line(input, &buffer, line,
                                                          thickness);

            // cprimim_draw_line(output_buffer_pointer, line, color, thickness);
            new_improvement = cprimim_line_improvement(input, output, &buffer,
                                                       line, color, thickness);

            if (new_improvement >= current_improvement) {
                line = old_line;
                number_of_tries++;

            } else {
                current_improvement = new_improvement;
                number_of_tries = 0;
            }
        }
        global_tries += total_tries;

        cprimim_draw_line(output, &buffer, line, color, thickness);
        // cprimim_set_image(output_buffer_pointer, output);
    }
    free(buffer.indices);
    printf("tries overall: %d\n", global_tries);
    return;
}
