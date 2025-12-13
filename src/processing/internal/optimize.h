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
    cprimim_##TYPE *shapes = context->state.shapes;\
    cprimim_##TYPE *candidate_shapes = context->state.candidate_shapes;\
    int columns = input->columns;\
\
    int rows = input->rows;\
    context->state.prof = (cprimim_Profiler){0};\
    TBEGIN(t_total);\
    cprimim_Color avg = cprimim_avg_color(&context->input);\
    cprimim_set_background(output, &avg);\
    double tries_per_shape = 0;\
    int accepted_shapes = 0;\
    bool rejected = false;\
    uint64_t total_grid_err = 0;\
    uint64_t reduced_since_grid = 0;\
    double rel_drop = 0;\
    double rel_thresh = 0.05;\
    int64_t start_improvement = 0;\
    int64_t best_improvement = 0;\
    update_grid_errors(&context->state, input, output, columns, rows, initial_shapes);\
    total_grid_err = total_grid_error(context->state.grid_errors, initial_shapes);\
    {\
        utils_srand(time(NULL) * 0x9E3779B97F4A7C15ULL);\
        for (int k = 0; k < number_of_lines; k++) {\
                TBEGIN(tg);\
                if(!rejected && rel_drop >= rel_thresh){\
                    update_grid_errors(&context->state, input, output, columns, rows, initial_shapes);\
                    total_grid_err = total_grid_error(context->state.grid_errors, initial_shapes);\
                    reduced_since_grid = 0;\
                    rel_drop = 0;\
                }\
                rejected = false;\
                TACCUM(context->state.prof.grid_ns, tg);\
                int tries_shape = 0;\
                cprimim_##TYPE old_candidate_shape = {0};\
                cprimim_##TYPE candidate_shape = {0};\
\
                candidate_shape.improvement = INT_MAX;\
                old_candidate_shape.improvement = INT_MAX;\
                candidate_shape.improvement_coarse = INT_MAX;\
                old_candidate_shape.improvement_coarse = INT_MAX;\
                context->state.prof.bestfit_calls++;\
                candidate_shape = pick_best_initial_##TYPE(&context->state,\
                    input, output, columns, rows, initial_shapes);\
                if(candidate_shape.improvement>=0){\
                    rejected = true;\
                    k--;\
                    continue;\
                }\
                old_candidate_shape = candidate_shape;\
                start_improvement = candidate_shape.improvement;\
                context->state.prof.sum_start_improvement += candidate_shape.improvement;\
                size_t number_of_tries = 0;\
                while (number_of_tries < max_number_of_tries) {\
                    context->state.prof.mutations_total++;\
                    tries_shape++;\
                    tries_per_shape++;\
                    mutate_##TYPE(&candidate_shape, input->columns,\
                                  input->rows);\
                    /*TBEGIN(tb);*/\
                    cprimim_best_fit(input, output, &candidate_shape, 4);\
                        /*TACCUM(context->state.prof.bestfit_ns, tb);*/\
                    if (candidate_shape.improvement_coarse >=\
                        old_candidate_shape.improvement_coarse) {\
                        candidate_shape = old_candidate_shape;\
                        number_of_tries++;\
                        /*context->state.prof.mutations_rejected++;*/\
                    \
                    }\
                    else{\
                        context->state.prof.bestfit_calls++;\
                    TBEGIN(tb);\
                        cprimim_best_fit(input, output, &candidate_shape, 1);\
                        TACCUM(context->state.prof.bestfit_ns, tb);\
                    if (candidate_shape.improvement >=\
                        old_candidate_shape.improvement) {\
                        candidate_shape = old_candidate_shape;\
                        number_of_tries++;\
                        context->state.prof.mutations_rejected++;\
                    \
                    }\
                        else {\
                        context->state.prof.mutations_accepted++;\
                        old_candidate_shape = candidate_shape;\
                        number_of_tries = 0;\
                    }\
                    }\
                }\
                best_improvement = candidate_shape.improvement;\
                context->state.prof.sum_best_improvement += candidate_shape.improvement;\
                context->state.prof.sum_delta_improvement += start_improvement-best_improvement;\
                context->state.prof.mut_hist[bucket_u64(tries_shape)]++;\
                accepted_shapes++;\
                reduced_since_grid += -candidate_shape.improvement;\
                rel_drop = reduced_since_grid/(double)total_grid_err;\
                shapes[k] = candidate_shape;\
                TBEGIN(td);\
                cprimim_draw_##TYPE(output, &shapes[k], shapes[k].color);\
                TACCUM(context->state.prof.draw_ns, td);\
                context->state.prof.shapes_done++;\
        }\
    }\
    TACCUM(context->state.prof.approx_ns, t_total);\
    cprimim_print_profile(&context->state.prof);\
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
                      cprimim_##TYPE *shape, int stride) {\
    cprimim_Comparator comparator = {0};\
    comparator.max_error = INT64_MAX;\
    comparator.other = output;\
    comparator.stride = stride;\
    improvement(image, shape, &comparator);\
    if (comparator.counter == 0) {\
        shape->improvement_coarse = 0; \
    return;\
    }\
    int64_t denom = A * comparator.counter;\
    shape->color.r = cprimim_clamp(-comparator.sum_diffs[0] / denom, 0, 255);\
    shape->color.g = cprimim_clamp(-comparator.sum_diffs[1] / denom, 0, 255);\
    shape->color.b = cprimim_clamp(-comparator.sum_diffs[2] / denom, 0, 255);\
    int64_t error_new =1*(\
        comparator.sum_diffs_squared[0]-\
        comparator.sum_diffs[0] * comparator.sum_diffs[0] / comparator.counter);\
    error_new += 1*(comparator.sum_diffs_squared[1]  - comparator.sum_diffs[1] *\
                                                       comparator.sum_diffs[1]  /\
                                                       comparator.counter);\
    error_new += comparator.sum_diffs_squared[2]  - comparator.sum_diffs[2] *\
                                                       comparator.sum_diffs[2]  /\
                                                       comparator.counter;\
    if(stride == 1){\
    shape->improvement = -(comparator.error_old- error_new/ (255 * 255));\
}\
    else{\
    shape->improvement_coarse = -(comparator.error_old- error_new/ (255 * 255));\
}\
}
#if 1
#define BEST_INITIAL(TYPE)\
static inline cprimim_##TYPE pick_best_initial_##TYPE(cprimim_OptState* state, cprimim_Image *orig,\
                                               cprimim_Image *curr, int columns,\
                                               int rows, int n_init) {\
\
    cprimim_##TYPE best = {0};\
    cprimim_##TYPE cand = {0};\
    best.improvement = INT64_MAX;\
    for(int k=0; k<8; k++){\
        int index = sample_grid(state, n_init);\
        cand =\
            random_##TYPE(index, n_init, columns, rows);\
        cprimim_best_fit(orig, curr, &cand, 1);\
        if(cand.improvement < best.improvement) best=cand;\
    }\
    return best;\
}
#else

#define BEST_INITIAL(TYPE)\
static inline cprimim_##TYPE pick_best_initial_##TYPE(cprimim_OptState* state, cprimim_Image *orig,\
                                               cprimim_Image *curr, int columns,\
                                               int rows, int n_init) {\
\
    int index = sample_grid(state, n_init);\
    cprimim_##TYPE cand =\
        random_##TYPE(index, n_init, columns, rows);\
    cprimim_best_fit(orig, curr, &cand);\
    return cand;\
}
#endif
#endif

