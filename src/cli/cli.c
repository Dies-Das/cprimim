#include "cprimim.h"
#include "image.h"
#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"
#include <flag.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
void usage(FILE *stream) {
    fprintf(stream, "Usage: ./example [OPTIONS] [--] <OUTPUT FILES...>\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}
int main(int argc, char *argv[]) {
    bool *help =
        flag_bool("help", false, "Print this help to stdout and exit with 0");
    uint64_t *thickness =
        flag_uint64("thickness", 3,
                    "Thickness of line for lines and bezier curves. Only "
                    "comparatively thin lines are supported right now.");
    uint64_t *width =
        flag_uint64("width", 500,
                    "Width of the image during processing. Full HD is not "
                    "necessary and hinders performance.");
    uint64_t *nr_of_shapes = flag_uint64(
        "n", 300,
        "Number of shapes to draw. For lines, a good number is around 2000.");
    uint64_t *nr_of_candidates =
        flag_uint64("c", 3, "Number of candidate shapes to consider.");
    uint64_t *nr_of_tries =
        flag_uint64("tries", 5,
                    "Will stop hill climbing if there was no improvement after "
                    "trying -tries times");
    char **input_path = flag_str("input", "in.png", "Image file to load");
    uint64_t *method = flag_uint64(
        "method", 0,
        "Method to use. line (0) and bezier(1) are implemented so far.");
    uint64_t *threads = flag_uint64("threads", 1, "Number of threads to use.");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    if (*help) {
        usage(stdout);
        exit(0);
    }
    printf("input path is %s", *input_path);
    int rest_argc = flag_rest_argc();
    char **rest_argv = flag_rest_argv();

    char *outfile = "out.png";
    if (rest_argc > 1) {
        usage(stderr);
        fprintf(stderr, "Too many output files provided!\n");
        exit(1);
    } else if (rest_argc == 1) {
        outfile = rest_argv[0];
    }
    int x = 0;
    int y = 0;
    int n = 0;
    char *result = argv[1];
    cprimim_Image input;
    input.data = stbi_load(*input_path, &input.columns, &input.rows, &n, 3);
    if (input.data == NULL) {
        printf("File could not be loaded!\n");
        return EXIT_FAILURE;
    }
    int processing_columns = *width;
    int processing_rows =
        ((double)input.rows / input.columns) * processing_columns;
    cprimim_Image resized_input;
    resized_input.columns = processing_columns;
    resized_input.rows = processing_rows;
    resized_input.data = stbir_resize_uint8_srgb(
        input.data, input.columns, input.rows, input.columns * 3, NULL,
        processing_columns, processing_rows, processing_columns * 3, STBIR_RGB);
    cprimim_Image small_output = {0};
    small_output = cprimim_copy_image(&resized_input);
    cprimim_Context context = cprimim_create_context(
        *method, *nr_of_shapes, *nr_of_candidates, *threads, *nr_of_tries,
        processing_columns, processing_rows);
    cprimim_set_input(&context, resized_input.data);
    printf("starting approximation..\n");
    double elapsed = 0;
    double time = clock();
    small_output = *cprimim_approximate(&context);
    elapsed = (double)clock() - time;
    printf("We have %f fps!\n", CLOCKS_PER_SEC / elapsed);
    cprimim_Image full_output = {0};
    full_output.data = stbir_resize_uint8_srgb(
        small_output.data, small_output.columns, small_output.rows,
        small_output.columns * 3, NULL, input.columns, input.rows,
        input.columns * 3, STBIR_RGB);
    printf("wrote full output\n");
    int write_succ = stbi_write_png(outfile, input.columns, input.rows, 3,
                                    full_output.data, input.columns * 3);
    printf("wrote png\n");
    printf("columns: %d\n", input.columns);
    stbi_image_free(input.data);
    stbi_image_free(resized_input.data);
    // stbi_image_free(small_output.data);
    cprimim_destroy_context(&context);
    // stbi_image_free(full_output.data);
    // stbi_image_free(output.data);
    return EXIT_SUCCESS;
}
