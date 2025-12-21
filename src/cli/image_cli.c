#include "image_cli.h"
#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"
#include "cprimim.h"
#include "args.h"
#include <stdlib.h>
#include <time.h>
int process_image(Args args){

    int in_w = 0, in_h = 0, in_comp = 0;
    uint8_t *input_rgb = stbi_load(*args.input_path, &in_w, &in_h, &in_comp, 4);
    if (!input_rgb)
    {
        fprintf(stderr, "Could not load input file: %s\n", *args.input_path);
        return EXIT_FAILURE;
    }
    int minimum_dimension = in_h > in_w ? in_w : in_h;
    double scale = (double)*args.size / minimum_dimension;
    const int proc_w = (int)(in_w * scale);
    const int proc_h = (int)(in_h * scale);
    uint8_t *proc_rgb = stbir_resize_uint8_srgb(input_rgb, in_w, in_h, in_w * 4, NULL, proc_w,
                                                proc_h, proc_w * 4, STBIR_RGBA);

    if (!proc_rgb)
    {
        fprintf(stderr, "Resize failed.\n");
        stbi_image_free(input_rgb);
        return EXIT_FAILURE;
    }

    cprimim_Context *ctx = cprimim_create_context(
        (enum cprimim_shape)(*args.method), (cprimim_BackgroundType)*args.background, (size_t)(*args.nr_of_shapes),
        (size_t)(*args.initial_cells), (size_t)(*args.nr_of_tries), proc_w, proc_h, *args.background_shapes, *args.alpha);

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
    if(!args.output_path){
        args.output_path = "out.svg";
    }

    FILE *ptr = fopen(args.output_path, "w");
    if(!ptr){
        fprintf(stderr, "Colud not open output file %s!", args.output_path);
        return EXIT_FAILURE;
    }
    cprimim_to_svg(ctx, ptr);
    fclose(ptr);
    cprimim_destroy_context(ctx);
    stbi_image_free(input_rgb);
    stbi_image_free(proc_rgb);
    return EXIT_SUCCESS;
}
