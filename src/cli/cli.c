#include "cprimim.h"
// #include "image.h"
#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"
#include <flag.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./example [OPTIONS] [--] <OUTPUT FILES...>\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}
int main(int argc, char *argv[])
{
    bool *help = flag_bool("help", false, "Print this help to stdout and exit with 0");
    uint64_t *size = flag_uint64("size", 500,
                                 "Minimum dimension of the image during processing. Full HD is not "
                                 "necessary and hinders performance.");
    uint64_t *nr_of_shapes =
        flag_uint64("n", 300, "Number of shapes to draw. For lines, a good number is around 2000.");
    uint64_t *nr_of_initial =
        flag_uint64("cells", 1000, "Number of grid cells for starting shape sampling");
    uint64_t *nr_of_tries = flag_uint64("tries", 5,
                                        "Will stop hill climbing if there was no improvement after "
                                        "trying -tries times");
    char **input_path = flag_str("input", "in.png", "Image file to load");
    uint64_t *method =
        flag_uint64("method", 0,
                    "Method to use. line (0), bezier (1) and triangle (2) are implemented so far.");

    uint64_t *background = flag_uint64(
        "bg", 0,
        "Background method to use. Options are uniform background(0) and triangulation(1)");
    uint64_t *background_shapes = flag_uint64(
        "n_background", 100, "Only relevant for triangulation. (Upper limit of) triangles used.");
    if (!flag_parse(argc, argv))
    {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    if (*help)
    {
        usage(stdout);
        exit(0);
    }
    int rest_argc = flag_rest_argc();
    char **rest_argv = flag_rest_argv();

    char *outfile = "out.png";
    if (rest_argc > 1)
    {
        usage(stderr);
        fprintf(stderr, "Too many output files provided!\n");
        exit(1);
    }
    else if (rest_argc == 1)
    {
        outfile = rest_argv[0];
    }
    printf("bg method is %i\n", *background);
    int in_w = 0, in_h = 0, in_comp = 0;
    uint8_t *input_rgb = stbi_load(*input_path, &in_w, &in_h, &in_comp, 3);
    if (!input_rgb)
    {
        fprintf(stderr, "Could not load input file: %s\n", *input_path);
        return EXIT_FAILURE;
    }
    int minimum_dimension = in_h > in_w ? in_w : in_h;
    double scale = (double) *size/minimum_dimension;
    const int proc_w = (int)(in_w*scale);
    const int proc_h = (int)(in_h*scale);

    uint8_t *proc_rgb = stbir_resize_uint8_srgb(input_rgb, in_w, in_h, in_w * 3, NULL, proc_w,
                                                proc_h, proc_w * 3, STBIR_RGB);

    if (!proc_rgb)
    {
        fprintf(stderr, "Resize failed.\n");
        stbi_image_free(input_rgb);
        return EXIT_FAILURE;
    }

    cprimim_Context *ctx = cprimim_create_context(
        (enum cprimim_shape)(*method), (cprimim_BackgroundType) *background, (size_t)(*nr_of_shapes), 
        (size_t)(*nr_of_initial), (size_t)(*nr_of_tries), proc_w, proc_h, *background_shapes);

    if (!ctx)
    {
        fprintf(stderr, "Failed to create cprimim context.\n");
        stbi_image_free(input_rgb);
        stbi_image_free(proc_rgb);
        return EXIT_FAILURE;
    }

    cprimim_set_input(ctx, proc_rgb);

    fprintf(stdout, "starting approximation..\n");
    clock_t t0 = clock();
    cprimim_Image *small_out = cprimim_approximate(ctx);
    clock_t t1 = clock();

    double elapsed = (double)(t1 - t0);
    if (elapsed > 0)
    {
        fprintf(stdout, "We have %f fps!\n", (double)CLOCKS_PER_SEC / elapsed);
    }

    FILE *ptr = fopen("test.svg", "w");
    cprimim_to_svg(ctx, ptr);
    fclose(ptr);
    uint8_t *small_out_rgb = cprimim_image_data(small_out);
    if (!small_out_rgb)
    {
        fprintf(stderr, "cprimim_image_data returned NULL.\n");
        cprimim_destroy_context(ctx);
        stbi_image_free(input_rgb);
        stbi_image_free(proc_rgb);
        return EXIT_FAILURE;
    }

    uint8_t *full_out_rgb = stbir_resize_uint8_srgb(small_out_rgb, proc_w, proc_h, proc_w * 3, NULL,
                                                    in_w, in_h, in_w * 3, STBIR_RGB);

    if (!full_out_rgb)
    {
        fprintf(stderr, "Upscale failed.\n");
        cprimim_destroy_context(ctx);
        stbi_image_free(input_rgb);
        stbi_image_free(proc_rgb);
        return EXIT_FAILURE;
    }

    int ok = stbi_write_png(outfile, in_w, in_h, 3, full_out_rgb, in_w * 3);
    if (!ok)
    {
        fprintf(stderr, "Failed to write PNG: %s\n", outfile);
        stbi_image_free(full_out_rgb);
        cprimim_destroy_context(ctx);
        stbi_image_free(input_rgb);
        stbi_image_free(proc_rgb);
        return EXIT_FAILURE;
    }
    stbi_image_free(full_out_rgb);
    cprimim_destroy_context(ctx);
    stbi_image_free(input_rgb);
    stbi_image_free(proc_rgb);

    return EXIT_SUCCESS;
}
