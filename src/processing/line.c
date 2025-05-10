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
    int err = dx - dy, e2, x2, y2;
    int ed = dx + dy == 0 ? 1 : (dx * dx + dy * dy);
    buffer->count = 0;
    size_t idx;

    int half = (thickness + 1) / 2;

    bool steep = (dy > dx);

    while (true) {
        if (steep) {
            for (int off = -half; off <= half; off++) {
                int xx = x0 + off;
                int yy = y0;
                if (xx >= 0 && yy >= 0 && xx < image->columns &&
                    yy < image->rows) {
                    idx = (size_t)CHANNELS * ((size_t)yy * image->columns + xx);
                    buffer->indices[buffer->count++] = idx;
                    cprimim_average_color_callback(image, xx, yy, &result);
                }
            }
        } else {
            for (int off = -half; off <= half; off++) {
                int xx = x0;
                int yy = y0 + off;
                if (xx >= 0 && yy >= 0 && xx < image->columns &&
                    yy < image->rows) {
                    idx = (size_t)CHANNELS * ((size_t)yy * image->columns + xx);
                    buffer->indices[buffer->count++] = idx;
                    cprimim_average_color_callback(image, xx, yy, &result);
                }
            }
        }

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
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

            // cprimim_draw_line(output_buffer_pointer, line, color,
            // thickness);
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
