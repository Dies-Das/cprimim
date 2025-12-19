#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#define MUTATION_MIN 2

// This is the optimizer macro. For a shape one needs to implement drawing and mutating.
#include "utils.h"
#define SHAPE_APPROX(TYPE)                                                                         \
    void cprimim_##TYPE##_approx(cprimim_Context *context)                                         \
    {                                                                                              \
        Image *input = &context->input;                                                            \
        Image *output = &context->output;                                                          \
        size_t number_of_lines = context->nr_shapes;                                               \
        size_t max_number_of_tries = context->attempts;                                            \
        size_t initial_shapes = context->initial_cells;                                            \
        shape *shapes = context->state.shapes;                                                     \
        int columns = input->columns;                                                              \
                                                                                                   \
        int rows = input->rows;                                                                    \
        int size = rows > columns ? columns : rows;                                                \
        TBEGIN(t_total);                                                                           \
        double tries_per_shape = 0;                                                                \
        int accepted_shapes = 0;                                                                   \
        bool rejected = false;                                                                     \
        uint64_t total_grid_err = 0;                                                               \
        uint64_t reduced_since_grid = 0;                                                           \
        double rel_drop = 0;                                                                       \
        double rel_thresh = 0.05;                                                                  \
        int64_t start_improvement = 0;                                                             \
        int64_t best_improvement = 0;                                                              \
        cprimim_update_grid_errors(&context->state, input, output, columns, rows, initial_shapes); \
        total_grid_err = total_grid_error(context->state.grid_errors, initial_shapes);             \
        {                                                                                          \
            utils_srand(time(NULL) * 0x9E3779B97F4A7C15ULL);                                       \
            for (int k = 0; k < number_of_lines;)                                                  \
            {                                                                                      \
                double mutation_radius = size * .1;                                                \
                TBEGIN(tg);                                                                        \
                if (!rejected && rel_drop >= rel_thresh)                                           \
                {                                                                                  \
                    cprimim_update_grid_errors(&context->state, input, output, columns, rows,      \
                                               initial_shapes);                                    \
                    total_grid_err = total_grid_error(context->state.grid_errors, initial_shapes); \
                    reduced_since_grid = 0;                                                        \
                    rel_drop = 0;                                                                  \
                }                                                                                  \
                rejected = false;                                                                  \
                TACCUM(context->state.prof.grid_ns, tg);                                           \
                int tries_shape = 0;                                                               \
                TYPE old_candidate_shape = {0};                                                    \
                TYPE candidate_shape = {0};                                                        \
                                                                                                   \
                candidate_shape.improvement = INT_MAX;                                             \
                old_candidate_shape.improvement = INT_MAX;                                         \
                candidate_shape.improvement_coarse = INT_MAX;                                      \
                old_candidate_shape.improvement_coarse = INT_MAX;                                  \
                context->state.prof.bestfit_calls++;                                               \
                candidate_shape =                                                                  \
                    pick_best_initial_##TYPE(&context->state, input, output, columns, rows,        \
                                             initial_shapes, (int)mutation_radius);                \
                if (candidate_shape.improvement_coarse >= 0)                                       \
                {                                                                                  \
                    rejected = true;                                                               \
                    continue;                                                                      \
                }                                                                                  \
                old_candidate_shape = candidate_shape;                                             \
                start_improvement = candidate_shape.improvement;                                   \
                context->state.prof.sum_start_improvement += candidate_shape.improvement;          \
                size_t number_of_tries = 0;                                                        \
                while (number_of_tries < max_number_of_tries)                                      \
                {                                                                                  \
                    CLAMP(mutation_radius, MUTATION_MIN, 0.5 * columns);                           \
                    context->state.prof.mutations_total++;                                         \
                    tries_shape++;                                                                 \
                    tries_per_shape++;                                                             \
                    mutate_##TYPE(&candidate_shape, input->columns, input->rows,                   \
                                  (int)mutation_radius);                                           \
                    /*TBEGIN(tb);*/                                                                \
                    TBEGIN(tb);                                                                    \
                    cprimim_best_fit_##TYPE(input, output, &candidate_shape, COARSE_STRIDE);       \
                    /*TACCUM(context->state.prof.bestfit_ns, tb);*/                                \
                    if (candidate_shape.improvement_coarse >=                                      \
                        old_candidate_shape.improvement_coarse)                                    \
                    {                                                                              \
                        candidate_shape = old_candidate_shape;                                     \
                        number_of_tries++;                                                         \
                        mutation_radius *= 1;                                                      \
                        /*if(mutation_radius = MUTATION_MIN) number_of_tries++;                    \
                        else number_of_tries=0;                                                    \
                        /* mutation_radius = clamp(mutation_radius, 10, .5*columns);               \
                        /*context->state.prof.mutations_rejected++;*/                              \
                    }                                                                              \
                    else                                                                           \
                    {                                                                              \
                        context->state.prof.bestfit_calls++;                                       \
                        cprimim_best_fit_##TYPE(input, output, &candidate_shape, 1);               \
                        if (candidate_shape.improvement >= old_candidate_shape.improvement)        \
                        {                                                                          \
                            candidate_shape = old_candidate_shape;                                 \
                            number_of_tries++;                                                     \
                            context->state.prof.mutations_rejected++;                              \
                            mutation_radius *= 1;                                                  \
                            /*mutation_radius = clamp(mutation_radius, 10, .5*columns);/**/        \
                        }                                                                          \
                        else                                                                       \
                        {                                                                          \
                            context->state.prof.mutations_accepted++;                              \
                            old_candidate_shape = candidate_shape;                                 \
                            number_of_tries = 0;                                                   \
                            mutation_radius *= 1;                                                  \
                            /*mutation_radius = clamp(mutation_radius, 10, .5*columns);/**/        \
                        }                                                                          \
                    }                                                                              \
                    TACCUM(context->state.prof.bestfit_ns, tb);                                    \
                    /* if(mutation_radius == MUTATION_MIN) break;*/                                \
                }                                                                                  \
                best_improvement = candidate_shape.improvement;                                    \
                context->state.prof.sum_best_improvement += candidate_shape.improvement;           \
                context->state.prof.sum_delta_improvement += start_improvement - best_improvement; \
                context->state.prof.mut_hist[bucket_u64(tries_shape)]++;                           \
                accepted_shapes++;                                                                 \
                reduced_since_grid += -candidate_shape.improvement;                                \
                rel_drop = reduced_since_grid / (double)total_grid_err;                            \
                shapes[k].shape.TYPE = candidate_shape;                                            \
                shapes[k].s = context->s;                                                          \
                TBEGIN(td);                                                                        \
                draw_##TYPE(output, &candidate_shape, candidate_shape.color);                      \
                TACCUM(context->state.prof.draw_ns, td);                                           \
                context->state.prof.shapes_done++;                                                 \
                k++;                                                                               \
            }                                                                                      \
        }                                                                                          \
        TACCUM(context->state.prof.approx_ns, t_total);                                            \
        print_profile(&context->state.prof);                                                       \
        return;                                                                                    \
    }
#define SORT(SHAPE)                                                                                \
    static inline void sort_##SHAPE(SHAPE *shapes, int n)                                          \
    {                                                                                              \
        for (int i = 1; i < n; i++)                                                                \
        {                                                                                          \
            SHAPE key = shapes[i];                                                                 \
            int j = i - 1;                                                                         \
            while (j >= 0 && shapes[j].improvement > key.improvement)                              \
            {                                                                                      \
                shapes[j + 1] = shapes[j];                                                         \
                j--;                                                                               \
            }                                                                                      \
            shapes[j + 1] = key;                                                                   \
        }                                                                                          \
    }
#define BEST_FIT(TYPE)                                                                             \
    void cprimim_best_fit_##TYPE(Image *image, Image *output, TYPE *shape, int stride)             \
    {                                                                                              \
        Comparator comparator = {0};                                                               \
        comparator.max_error = INT64_MAX;                                                          \
        comparator.other = output;                                                                 \
        comparator.stride = stride;                                                                \
        improvement(image, shape, &comparator);                                                    \
        if (stride > 1 && comparator.counter < 32)                                                 \
        {                                                                                          \
            shape->improvement_coarse = 0;                                                         \
            return;                                                                                \
        }                                                                                          \
        int64_t denom = A * comparator.counter;                                                    \
        shape->color.r = clamp(-comparator.sum_diffs[0] / denom, 0, 255);                          \
        shape->color.g = clamp(-comparator.sum_diffs[1] / denom, 0, 255);                          \
        shape->color.b = clamp(-comparator.sum_diffs[2] / denom, 0, 255);                          \
        int64_t error_new =                                                                        \
            1 * (comparator.sum_diffs_squared[0] -                                                 \
                 comparator.sum_diffs[0] * comparator.sum_diffs[0] / comparator.counter);          \
        error_new += 1 * (comparator.sum_diffs_squared[1] -                                        \
                          comparator.sum_diffs[1] * comparator.sum_diffs[1] / comparator.counter); \
        error_new += comparator.sum_diffs_squared[2] -                                             \
                     comparator.sum_diffs[2] * comparator.sum_diffs[2] / comparator.counter;       \
        if (stride == 1)                                                                           \
        {                                                                                          \
            shape->improvement = -(comparator.error_old - error_new / (255 * 255));                \
            shape->residual = error_new;                                                           \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            shape->improvement_coarse = -(comparator.error_old - error_new / (255 * 255));         \
        }                                                                                          \
    }
#if 1
#define BEST_INITIAL(TYPE)                                                                         \
    static inline TYPE pick_best_initial_##TYPE(OptState *state, Image *orig, Image *curr,         \
                                                int columns, int rows, int n_init,                 \
                                                int mutation_radius)                               \
    {                                                                                              \
                                                                                                   \
        TYPE best = {0};                                                                           \
        TYPE cand = {0};                                                                           \
        best.improvement = INT64_MAX;                                                              \
        for (int k = 0; k < 32; k++)                                                               \
        {                                                                                          \
            int index = cprimim_sample_grid(state, n_init);                                        \
            cand = random_##TYPE(index, n_init, columns, rows, mutation_radius);                   \
            cprimim_best_fit_##TYPE(orig, curr, &cand, 2);                                         \
            if (cand.improvement_coarse < best.improvement_coarse)                                 \
                best = cand;                                                                       \
        }                                                                                          \
        return best;                                                                               \
    }
#else

#define BEST_INITIAL(TYPE)                                                                         \
    static inline##TYPE pick_best_initial_##TYPE(OptState *state, Image *orig, Image *curr,        \
                                                 int columns, int rows, int n_init)                \
    {                                                                                              \
                                                                                                   \
        int index = sample_grid(state, n_init);                                                    \
        ##TYPE cand = random_##TYPE(index, n_init, columns, rows);                                 \
        best_fit(orig, curr, &cand);                                                               \
        return cand;                                                                               \
    }
#endif
#endif
