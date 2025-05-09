#ifndef IMAGE_H
#define IMAGE_H
#define CHANNELS 3
#include "color.h"
typedef struct {
        unsigned char *data;
        int columns;
        int rows;
} cprimim_Image;
cprimim_Image cprimim_copy_image(const cprimim_Image *input);
void cprimim_set_image(const cprimim_Image *input, cprimim_Image *output);
void cprimim_draw_pixel(cprimim_Image *image, int x, int y,
                        cprimim_Color color);
double cprimim_mse(const cprimim_Image *image1, const cprimim_Image *image2);
cprimim_Color cprimim_avg_color(const cprimim_Image *image);
void cprimim_set_background(cprimim_Image *image, const cprimim_Color *color);
void cprimim_draw_pixel_callback(cprimim_Image *image, int x, int y,
                                 void *data);
void cprimim_average_color_callback(cprimim_Image *image, int x, int y,
                                    void *data);
void cprimim_compare_pixel_callback(cprimim_Image *image, int x, int y,
                                    void *data);
#endif // !IMAGE_H
