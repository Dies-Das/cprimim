#include "cprimim.h"
#include "image.h"

#include "stb_image.h"

#include "stb_image_resize2.h"

#include "stb_image_write.h"

#include <stdio.h>

#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int x = 0;
    int y = 0;
    int n = 0;

    srand(time(NULL));

    char *result = argv[1];
    cprimim_Image input;

    input.data = stbi_load(result, &input.columns, &input.rows, &n, 3);
    if (input.data == NULL) {
        printf("File could not be loaded!\n");
        return EXIT_FAILURE;
    }

    int processing_columns = 600;

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

    cprimim_line_approx(&resized_input, &small_output, 2000, 10, 6, NULL);

    int write_succ = stbi_write_png("hahaha_resize.png", small_output.columns,

                                    small_output.rows, 3, small_output.data,

                                    small_output.columns * 3);

    write_succ = stbi_write_png("hahaha.png", input.columns, input.rows, 3,

                                input.data, input.columns * 3);

    printf("Im write code: %d", write_succ);

    printf("columns: %d", input.columns);

    stbi_image_free(input.data);

    stbi_image_free(resized_input.data);

    stbi_image_free(small_output.data);

    // stbi_image_free(output.data);
    return EXIT_SUCCESS;
}
