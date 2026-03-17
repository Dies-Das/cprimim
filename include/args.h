#ifndef ARGS_H
#define ARGS_H
#include <stdint.h>
#include <stdbool.h>
typedef struct{
    bool *video;
    bool *help;
    uint64_t *size;
    uint64_t *alpha;
    uint64_t *nr_of_shapes;
    uint64_t *initial_cells;
    uint64_t *nr_of_tries;
    uint64_t *method;
    uint64_t *background;
    uint64_t *background_shapes;
    char ** input_path;
    char * output_path;

} Args;

#endif
