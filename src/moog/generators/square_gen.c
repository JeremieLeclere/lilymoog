/***************************************************************************************************
 * @file square_gen.c
 *
 * @brief Square waveform generation module (sources)
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

#include <square_gen.h>

#define QS823_MAX           ((1 << 23) - 1)         ///< QS8.23 scale factor


struct square_gen {
    float f0;                                       ///< Waveform frequency
    float fs;                                       ///< Sampling frequency
    float intensity;                                ///< Waveform intensity
    int index;                                      ///< Current index
    int32_t i_up;                                   ///< Square up value
    int32_t i_down;                                 ///< Square down value
    int32_t i_current;                              ///< Current value
    int32_t half_period;                            ///< Half square signal period
};


struct square_gen *square_gen_create(struct square_gen_params *params)
{
    struct square_gen *handle = NULL;

    if ((!params)
    ||  (params->fs < 0)
    ||  (params->f0 < 0)
    ||  (params->f0 >= params->fs/2)
    ||  (params->intensity < 0)
    ||  (params->intensity > 1))
        goto failure;

    handle = (struct square_gen *)calloc(1, sizeof(struct square_gen));
    if (!handle)
        goto failure;

    handle->f0          = params->f0;
    handle->fs          = params->fs;
    handle->intensity   = params->intensity;
    handle->i_up        = (int32_t)(handle->intensity * QS823_MAX);
    handle->i_down      = - (int32_t)(handle->intensity * QS823_MAX);
    handle->half_period = (int32_t)(handle->fs / (2 * handle->f0));
    handle->i_current   = handle->i_up;

    return handle;

failure:

    square_gen_destroy(&handle);

    return NULL;
}


void square_gen_destroy(struct square_gen **handle)
{
    if ((!handle) || (!(*handle)))
        goto exit;

    free(*handle);
    *handle = NULL;

exit:

    return;
}


int square_gen_get_frequency(struct square_gen *handle, float *f0)
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


int square_gen_set_frequency(struct square_gen *handle, float f0)
{
    int ret = 0;

    if ((!handle)
    ||  (f0 < 0)
    ||  (f0 >= handle->fs/2)) {
        ret = -EINVAL;
        goto exit;
    }

    /* Update square generation parameters */
    handle->f0          = f0;
    handle->half_period = (int32_t)(handle->fs / (2 * handle->f0));

    /* Current value update: Just reset index, and stay on the same state */
    handle->index       = 0;

exit:

    return ret;
}


int square_gen_get_intensity(struct square_gen *handle, float *intensity)
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


int square_gen_set_intensity(struct square_gen *handle, float intensity)
{
    int ret = 0;
    int up_or_down;

    if ((!handle)
    ||  (intensity < 0)
    ||  (intensity > 1)) {
        ret = -EINVAL;
        goto exit;
    }

    up_or_down = (handle->i_current == handle->i_up) ? 1 : -1;

    /* Update square generation parameters */
    handle->intensity   = intensity;
    handle->i_up        = (int32_t)(handle->intensity * QS823_MAX);
    handle->i_down      = - (int32_t)(handle->intensity * QS823_MAX);

    /* Current value update: Just update scale */
    handle->i_current = (up_or_down == 1) ? handle->i_up : handle->i_down;

exit:

    return ret;
}


int square_gen_process(struct square_gen *handle, int nb_frames, int32_t *out)
{
    int i, ret = 0;

    if ((!handle) || (!out)) {
        ret = -EINVAL;
        goto exit;
    }

    for (i = 0; i < nb_frames; i++) {
        out[i] = handle->i_current;
        handle->index++;
        if (handle->index == handle->half_period) {
            handle->index = 0;
            if (handle->i_current == handle->i_up)
                handle->i_current = handle->i_down;
            else
                handle->i_current = handle->i_up;
        }
    }

exit:

    return ret;
}
