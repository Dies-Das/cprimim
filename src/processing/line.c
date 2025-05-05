#include "line.h"
#include "cprimim.h"
#include "image.h"
#include "point.h"
#include "utils.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
static void cprimim_draw_line_no_antialias(cprimim_Image *image,
                                           cprimim_Line line,
                                           cprimim_Color color, int thickness);
static void cprimim_draw_perp(cprimim_Image *image, int cx, int cy, float px,
                              float py, int thickness, cprimim_Color color);
void cprimim_line_approx(const cprimim_Image *input, cprimim_Image *output,
                         int number_of_lines, int max_number_of_tries,
                         int thickness, cprimim_Image *output_buffer_pointer) {
    cprimim_Line line = {0};
    cprimim_Line old_line = {0};
    cprimim_Image output_buffer = {0};
    cprimim_Color avg = cprimim_avg_color(input);
    cprimim_set_background(output, &avg);
    if (output_buffer_pointer == NULL) {
        output_buffer = cprimim_copy_image(output);
        output_buffer_pointer = &output_buffer;
    } else {
        cprimim_set_image(output, output_buffer_pointer);
    }
    for (int k = 0; k < number_of_lines; k++) {
        printf("Doing line number %d\n", k);
        double current_error = cprimim_mse(input, output);
        double new_error = current_error;
        int number_of_tries = 0;
        while (number_of_tries < max_number_of_tries) {
            old_line = line;
            cprimim_set_image(output, output_buffer_pointer);
            cprimim_randomize_line(&line, input->columns, input->rows);
            cprimim_Color color =
                cprimim_average_color_line(input, line, thickness);
            cprimim_draw_line(output_buffer_pointer, line, color, thickness);
            new_error = cprimim_mse(input, output_buffer_pointer);
            if (new_error >= current_error) {
                line = old_line;
                number_of_tries++;
            } else {
                current_error = new_error;
                number_of_tries = 0;
            }
        }
        cprimim_set_image(output_buffer_pointer, output);
    }
    return;
}
void cprimim_randomize_line(cprimim_Line *line, int columns, int rows) {
    do {
        cprimim_randomize_point(&line->points[0], columns, rows);
        cprimim_randomize_point(&line->points[1], columns, rows);
    } while (cprimim_dot(line->points[0], line->points[1]) <
             -(double)cprimim_max(-rows, -columns) / 50);
}
void cprimim_mutate_line(cprimim_Line *line, int columns, int rows) {
    int index = cprimim_uniform_distribution(0, 2);
    cprimim_randomize_point(&line->points[index], columns, rows);
}
void cprimim_draw_line(cprimim_Image *image, cprimim_Line line,
                       cprimim_Color color, int thickness) {
    cprimim_draw_line_no_antialias(image, line, color, thickness);
}
static void cprimim_draw_line_no_antialias(cprimim_Image *image,
                                           cprimim_Line line,
                                           cprimim_Color color, int thickness) {
    int x0 = line.points[0].x;
    int y0 = line.points[0].y;
    int x1 = line.points[1].x;
    int y1 = line.points[1].y;
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = cprimim_sign(x1 - x0);
    int sy = cprimim_sign(y1 - y0);
    // Compute true perpendicular (normalized vector)
    float len = sqrtf((float)(dx * dx + dy * dy));
    float px = -(y1 - y0) / len;
    float py = (x1 - x0) / len;
    // Offset endpoints by half thickness perpendicular
    int half_thick = thickness / 2;
    float x_start = x0 + half_thick * px;
    float y_start = y0 + half_thick * py;
    float x = x_start;
    float y = y_start;
    // Outer Bresenham loop (main direction)
    int steps = cprimim_max(dx, dy);
    float x_inc = (float)(x1 - x0) / steps;
    float y_inc = (float)(y1 - y0) / steps;
    for (int i = 0; i <= steps; i++) {
        cprimim_draw_perp(image, roundf(x), roundf(y), px, py, thickness,
                          color);
        x += x_inc;
        y += y_inc;
    }
}
static void cprimim_draw_perp(cprimim_Image *image, int cx, int cy, float px,
                              float py, int thickness, cprimim_Color color) {
    int half = thickness / 2;
    for (int i = -half; i <= half; i++) {
        int xi = roundf(cx + i * px);
        int yi = roundf(cy + i * py);
        if (xi >= 0 && xi < image->columns && yi >= 0 && yi < image->rows)
            cprimim_draw_pixel(image, xi, yi, color);
    }
}
cprimim_Color cprimim_average_color_line(const cprimim_Image *image,
                                         cprimim_Line line, int thickness) {
    uint64_t sum_r = 0, sum_g = 0, sum_b = 0;
    size_t count = 0;
    int x0 = line.points[0].x;
    int y0 = line.points[0].y;
    int x1 = line.points[1].x;
    int y1 = line.points[1].y;
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    // signs for stepping along the line
    int sx = cprimim_sign(x1 - x0);
    int sy = cprimim_sign(y1 - y0);
    // compute true perpendicular (unit) vector:
    float len = sqrtf((float)(dx * dx + dy * dy));
    float px = -(y1 - y0) / len;
    float py = (x1 - x0) / len;
    // offset start point by half thickness along that perp:
    int half_thick = thickness / 2;
    float x = x0 + half_thick * px;
    float y = y0 + half_thick * py;
    // how many steps along the line:
    int steps = cprimim_max(dx, dy);
    float x_inc = (float)(x1 - x0) / (float)steps;
    float y_inc = (float)(y1 - y0) / (float)steps;
    for (int i = 0; i <= steps; i++) {
        // sample the “scanline” of width=thickness at (round(x),round(y))
        int cx = (int)roundf(x);
        int cy = (int)roundf(y);
        // walk perpendicular:
        for (int t = -half_thick; t <= half_thick; t++) {
            int xi = (int)roundf(cx + t * px);
            int yi = (int)roundf(cy + t * py);
            if (xi >= 0 && xi < image->columns && yi >= 0 && yi < image->rows) {
                // image->data is row‑major RGB(A)
                uint8_t *p = &image->data[(yi * image->columns + xi) * 3];
                sum_r += p[0];
                sum_g += p[1];
                sum_b += p[2];
                ++count;
            }
        }
        x += x_inc;
        y += y_inc;
    }
    cprimim_Color avg = {0, 0, 0};
    if (count > 0) {
        avg.r = (uint8_t)(sum_r / count);
        avg.g = (uint8_t)(sum_g / count);
        avg.b = (uint8_t)(sum_b / count);
    }
    return avg;
}
