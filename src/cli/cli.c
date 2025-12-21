#include "args.h"
#include "image_cli.h"
#include "video_cli.h"
#include <flag.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./example [OPTIONS] [--] <OUTPUT FILES...>\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}
int main(int argc, char *argv[])
{
    Args args = {0};
    bool *help = flag_bool("help", false, "Print this help to stdout and exit with 0");
    bool *video = flag_bool("video", false, "Process a video file");
    args.size = flag_uint64("size", 500,
                            "Minimum dimension of the image during processing. Full HD is not "
                            "necessary and hinders performance.");
    args.alpha = flag_uint64("alpha", 128, "alpha blending used");
    args.nr_of_shapes =
        flag_uint64("n", 300, "Number of shapes to draw. For lines, a good number is around 2000.");
    args.initial_cells =
        flag_uint64("cells", 1000, "Number of grid cells for starting shape sampling");
    args.nr_of_tries = flag_uint64("tries", 5,
                                   "Will stop hill climbing if there was no improvement after "
                                   "trying -tries times");
    args.input_path = flag_str("input", "in.png", "Image file to load");
    args.method =
        flag_uint64("method", 0,
                    "Method to use. line (0), bezier (1) and triangle (2) are implemented so far.");

    args.background = flag_uint64(
        "bg", 0,
        "Background method to use. Options are uniform background(0) and triangulation(1)");
    args.background_shapes = flag_uint64(
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
    if (rest_argc > 1)
    {
        usage(stderr);
        fprintf(stderr, "Too many output files provided!\n");
        exit(1);
    }
    else if (rest_argc == 1)
    {
        args.output_path = rest_argv[0];
    }

    if (*video)
    {
        return process_video(args);
    }
    else
    {
        return process_image(args);
    }
    return EXIT_SUCCESS;
}
