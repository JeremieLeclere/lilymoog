/***************************************************************************************************
 * @file saw_gen.c
 *
 * @brief Saw waveform generation module (sources)
 *
 * @licence MIT License
 *
 * Copyright (c) 2019 Jeremie Leclere
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#include <stdlib.h>

#include <saw_gen.h>

#define QS823_MAX           ((1 << 23) - 1)         ///< QS8.23 scale factor


struct saw_gen {
    float f0;                                       ///< Waveform frequency
    float fs;                                       ///< Sampling frequency
    float phase;                                    ///< Initial waveform phase
    float intensity;                                ///< Waveform intensity
    int32_t step;                                   ///< Decreasing step value
    int32_t i_max;                                  ///< Maximum intensity value
    int32_t i_min;                                  ///< Minimum intensity value
    int32_t i_current;                              ///< Current waveform value
};


struct saw_gen *saw_gen_create(struct saw_gen_params *params)
{
    struct saw_gen *handle = NULL;

    if ((!params)
    ||  (params->fs < 0)
    ||  (params->f0 < 0)
    ||  (params->f0 >= params->fs/2)
    ||  (params->intensity < 0)
    ||  (params->intensity > 1))
        goto failure;

    handle = (struct saw_gen *)calloc(1, sizeof(struct saw_gen));
    if (!handle)
        goto failure;

    handle->f0        = params->f0;
    handle->fs        = params->fs;
    handle->intensity = params->intensity;
    handle->i_max     = (int32_t)(handle->intensity * QS823_MAX);
    handle->i_min     = - (int32_t)(handle->intensity * QS823_MAX);
    handle->step      = (int32_t)((2 * handle->i_max) * (handle->f0 / handle->fs));
    handle->i_current = handle->i_max;

    return handle;

failure:

    saw_gen_destroy(&handle);

    return NULL;
}


void saw_gen_destroy(struct saw_gen **handle)
{
    if ((!handle) || (!(*handle)))
        goto exit;

    free(*handle);
    *handle = NULL;

exit:

    return;
}


int saw_gen_get_frequency(struct saw_gen *handle, float *f0)
{
    int ret = 0;

    if ((!handle)
    ||  (!f0)) {
        ret = -EINVAL;
        goto exit;
    }

    *f0 = handle->f0;

exit:

    return ret;
}


int saw_gen_set_frequency(struct saw_gen *handle, float f0)
{
    int ret = 0;

    if ((!handle)
    ||  (f0 < 0)
    ||  (f0 > handle->fs)) {
        ret = -EINVAL;
        goto exit;
    }

    /* Update saw generation parameters */
    handle->f0        = f0;
    handle->step      = (int32_t)((2 * handle->i_max) * (handle->f0 / handle->fs));

    /* No need to change current value: we just keep following at a diffrent rate */

exit:

    return ret;
}


int saw_gen_get_intensity(struct saw_gen *handle, float *intensity)
{
    int ret = 0;

    if ((!handle)
    ||  (!intensity)) {
        ret = -EINVAL;
        goto exit;
    }

    *intensity = handle->intensity;

exit:

    return ret;
}


int saw_gen_set_intensity(struct saw_gen *handle, float intensity)
{
    int ret = 0;

    if ((!handle)
    ||  (intensity < 0)
    ||  (intensity > 1)) {
        ret = -EINVAL;
        goto exit;
    }

    /* Update saw generation parameters */
    handle->intensity = intensity;
    handle->i_max     = (int32_t)(handle->intensity * QS823_MAX);
    handle->i_min     = - (int32_t)(handle->intensity * QS823_MAX);
    handle->step      = (int32_t)((2 * handle->i_max) * (handle->f0 / handle->fs));

    /* No need to change current value: we just keep following at a diffrent rate */

exit:

    return ret;
}


int saw_gen_process(struct saw_gen *handle, int nb_frames, int32_t *out)
{
    int i, ret = 0;
    int32_t underflow;

    if ((!handle) || (!out)) {
        ret = -EINVAL;
        goto exit;
    }

    for (i = 0; i < nb_frames; i++) {

        out[i] = handle->i_current;
        handle->i_current -= handle->step;
        if (handle->i_current < handle->i_min) {
            underflow = handle->i_current - handle->i_min;
            handle->i_current = handle->i_max - underflow;
        }
    }

exit:

    return ret;
}
