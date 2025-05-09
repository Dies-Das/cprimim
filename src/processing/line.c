#include "line.h"
#include "color.h"
#include "cprimim.h"
#include "image.h"
#include "point.h"
#include "utils.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
void cprimim_process_line(cprimim_Image *image, cprimim_Line line,
                          int thickness,
                          void (*callback)(cprimim_Image *, int, int, void *),
                          void *data);
static void cprimim_draw_line_no_antialias(cprimim_Image *image,
                                           cprimim_Line line,
                                           cprimim_Color color, int thickness);
// static void cprimim_draw_perp(cprimim_Image *image, int cx, int cy, float px,
//                               float py, int thickness, cprimim_Color color);
void cprimim_line_approx(cprimim_Image *input, cprimim_Image *output,
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
    cprimim_randomize_point(&line->points[0], columns, rows);
    cprimim_randomize_point(&line->points[1], columns, rows);
}
void cprimim_mutate_line(cprimim_Line *line, int columns, int rows) {
    int index = cprimim_uniform_distribution(0, 2);
    cprimim_randomize_point(&line->points[index], columns, rows);
}
void cprimim_draw_line(cprimim_Image *image, cprimim_Line line,
                       cprimim_Color color, int thickness) {
    // cprimim_draw_line_no_antialias(image, line, color, thickness);
    cprimim_process_line(image, line, thickness, cprimim_draw_pixel_callback,
                         &color);
}
cprimim_Color cprimim_average_color_line(cprimim_Image *image,
                                         cprimim_Line line, int thickness) {
    cprimim_Color result = {0};
    cprimim_AvgColor avg = {0};
    cprimim_process_line(image, line, thickness, cprimim_average_color_callback,
                         &avg);
    result.r = avg.r / avg.count;
    result.g = avg.g / avg.count;
    result.b = avg.b / avg.count;
    return result;
}
void cprimim_process_line(cprimim_Image *image, cprimim_Line line,
                          int thickness,
                          void (*callback)(cprimim_Image *, int, int, void *),
                          void *data) {
    int x0 = line.points[0].x;
    int x1 = line.points[1].x;
    int y0 = line.points[0].y;
    int y1 = line.points[1].y;
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx - dy, e2, x2, y2; /* error value e_xy */
    int ed = dx + dy == 0 ? 1 : (dx * dx + dy * dy);
    int r = 0;
    int g = 0;
    int b = 0;
    size_t index = 0;
    size_t count = 0;
    for (thickness = (thickness + 1) / 2;;) { /* pixel loop */
        assert(x0 >= 0 && y0 >= 0 && x0 < image->columns && y0 < image->rows);
        if (x0 >= 0 && y0 >= 0 && x0 < image->columns && y0 < image->rows) {
            callback(image, x0, y0, data);
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
                    callback(image, x2, y0, data);
                }
            }
            if (x0 == x1)
                break;
            e2 = err;
            err -= dy;
            x0 += sx;
        }
        if (2 * e2 <= dy) { /* y step */
            for (e2 = dx - e2;
                 e2 * e2 < ed * thickness * thickness && (x1 != x2 || dx < dy);
                 e2 += dy) {
                x2 += sx;
                if (x0 >= 0 && y2 >= 0 && x0 < image->columns &&
                    y2 < image->rows) {
                    callback(image, x0, y2, data);
                }
            }
            if (y0 == y1)
                break;
            err += dx;
            y0 += sy;
        }
    }
}
