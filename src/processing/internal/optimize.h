#ifndef OPTIMIZE_H
#define OPTIMIZE_H


// This is the optimizer macro. For a shape one needs to implement drawing and mutating.
#define SHAPE_APPROX(TYPE)\
void cprimim_##TYPE##_approx(cprimim_Context *context) {\
    cprimim_Image *input = &context->input;\
    cprimim_Image *output = &context->output;\
    size_t number_of_lines = context->nr_shapes;\
    size_t max_number_of_tries = context->attempts;\
    size_t initial_shapes = context->initial_shapes;\
    cprimim_##TYPE *shapes = context->shapes;\
    cprimim_##TYPE *candidate_shapes = context->candidate_shapes;\
    int columns = input->columns;\
\
    int rows = input->rows;\
    cprimim_Color avg = cprimim_avg_color(&context->input);\
    cprimim_set_background(output, &avg);\
    {\
        utils_srand(time(NULL) * 0x9E3779B97F4A7C15ULL);\
        for (int k = 0; k < number_of_lines; k++) {\
            for (int candidate = 0; candidate < context->candidates;\
                 candidate++) {\
                cprimim_##TYPE old_candidate_shape = {0};\
                cprimim_##TYPE candidate_shape = {0};\
\
                candidate_shape.improvement = INT_MAX;\
                old_candidate_shape.improvement = INT_MAX;\
                candidate_shape = pick_best_initial_##TYPE(\
                    input, output, columns, rows, initial_shapes);\
                size_t number_of_tries = 0;\
                while (number_of_tries < max_number_of_tries) {\
                    mutate_##TYPE(&candidate_shape, input->columns,\
                                  input->rows);\
                    cprimim_best_fit(input, output, &candidate_shape);\
                    if (candidate_shape.improvement >=\
                        old_candidate_shape.improvement) {\
                        candidate_shape = old_candidate_shape;\
                        number_of_tries++;\
\
                    } else {\
                        old_candidate_shape = candidate_shape;\
                        number_of_tries = 0;\
                    }\
                }\
                if(candidate_shape.improvement>=0){\
                    candidate--;\
                }\
                else{\
                candidate_shapes[candidate] = candidate_shape;\
                }\
            }\
            {\
                sort_##TYPE(candidate_shapes, context->candidates);\
                shapes[k] = candidate_shapes[0];\
                cprimim_draw_##TYPE(output, &shapes[k], shapes[k].color);\
            }\
        }\
    }\
    return;\
}
#define SORT(SHAPE)\
void sort_##SHAPE(cprimim_##SHAPE *shapes, int n) {\
    for (int i = 1; i < n; i++) {\
        cprimim_##SHAPE key = shapes[i];\
        int j = i - 1;\
        while (j >= 0 && shapes[j].improvement > key.improvement) {\
            shapes[j + 1] = shapes[j];\
            j--;\
        }\
        shapes[j + 1] = key;\
    }\
}
#define BEST_FIT(TYPE)\
static void cprimim_best_fit(cprimim_Image *image, cprimim_Image *output,\
                      cprimim_##TYPE *shape) {\
    cprimim_Comparator comparator = {0};\
    comparator.other = output;\
    improvement(image, shape, &comparator);\
    int64_t denom = A * comparator.counter;\
    shape->color.r = cprimim_clamp(-comparator.sum_diffs[0] / denom, 0, 255);\
    shape->color.g = cprimim_clamp(-comparator.sum_diffs[1] / denom, 0, 255);\
    shape->color.b = cprimim_clamp(-comparator.sum_diffs[2] / denom, 0, 255);\
    int64_t error_new =\
        comparator.sum_diffs_squared[0] -\
        comparator.sum_diffs[0] * comparator.sum_diffs[0] / comparator.counter;\
    error_new += comparator.sum_diffs_squared[1] - comparator.sum_diffs[1] *\
                                                       comparator.sum_diffs[1] /\
                                                       comparator.counter;\
    error_new += comparator.sum_diffs_squared[2] - comparator.sum_diffs[2] *\
                                                       comparator.sum_diffs[2] /\
                                                       comparator.counter;\
    shape->improvement = -(comparator.error_old - error_new / (255 * 255));\
}
#define BEST_INITIAL(TYPE)\
static inline cprimim_##TYPE pick_best_initial_##TYPE(cprimim_Image *orig,\
                                               cprimim_Image *curr, int columns,\
                                               int rows, int n_init) {\
    cprimim_##TYPE best = {.improvement = INT_MAX};\
\
    for (int i = 0; i < n_init; i++) {\
        cprimim_##TYPE cand =\
            random_##TYPE(i, n_init, columns, rows);\
        cprimim_best_fit(orig, curr, &cand);\
        if (cand.improvement < best.improvement) {\
            best = cand;\
        }\
    }\
    return best;\
}

#endif

