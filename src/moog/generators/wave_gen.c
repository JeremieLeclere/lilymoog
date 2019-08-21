/***************************************************************************************************
 * @file wave_gen.c
 *
 * @brief Waveform generation module (sources)
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
#include <sine_gen.h>
#include <wave_gen.h>
#include <square_gen.h>


struct wave_gen {
    void *gen;                          ///< Specific waveform generator handle
    float frequency;                    ///< Current frequency
    enum wave_gen_mode mode;            ///< Waveform type
};


struct wave_gen *wave_gen_create(struct wave_gen_params *params)
{
    struct wave_gen *handle = NULL;
    struct saw_gen_params saw_params;
    struct sine_gen_params sine_params;
    struct square_gen_params square_params;

    if (!params)
        goto failure;

    handle = (struct wave_gen *)calloc(1, sizeof(struct wave_gen));
    if (!handle)
        goto failure;

    handle->mode = params->mode;

    switch (handle->mode) {
    case WAVE_MODE_SAW:
        saw_params.f0        = params->f0;
        saw_params.fs        = params->fs;
        saw_params.intensity = params->intensity;
        handle->gen = (void *)saw_gen_create(&saw_params);
        break;
    case WAVE_MODE_SINE:
        sine_params.f0        = params->f0;
        sine_params.fs        = params->fs;
        sine_params.intensity = params->intensity;
        handle->gen = (void *)sine_gen_create(&sine_params);
        break;
    case WAVE_MODE_SQUARE:
        square_params.f0        = params->f0;
        square_params.fs        = params->fs;
        square_params.intensity = params->intensity;
        handle->gen = (void *)square_gen_create(&square_params);
        break;
    default:
        goto failure;
    }

    if (!handle->gen)
        goto failure;

    return handle;

failure:

    wave_gen_destroy(&handle);

    return NULL;
}


void wave_gen_destroy(struct wave_gen **handle)
{
    if ((!handle) || (!(*handle))) 
        goto exit;

    switch ((*handle)->mode) {
    case WAVE_MODE_SAW:
        saw_gen_destroy((struct saw_gen **)&(*handle)->gen);
        break;
    case WAVE_MODE_SINE:
        sine_gen_destroy((struct sine_gen **)&(*handle)->gen);
        break;
    case WAVE_MODE_SQUARE:
    default:
        square_gen_destroy((struct square_gen **)&(*handle)->gen);
        break;
    }

    free(*handle);
    *handle = NULL;

exit:

    return;
}


int wave_gen_get_frequency(struct wave_gen *handle, float *f0)
{
    int ret = 0;

    if ((!handle)
    ||  (!f0)) {
        ret = -EINVAL;
        goto exit;
    }

    switch (handle->mode) {
    case WAVE_MODE_SAW:
        ret = saw_gen_get_frequency((struct saw_gen *)handle->gen, f0);
        break;
    case WAVE_MODE_SINE:
        ret = sine_gen_get_frequency((struct sine_gen *)handle->gen, f0);
        break;
    case WAVE_MODE_SQUARE:
    default:
        ret = square_gen_get_frequency((struct square_gen *)handle->gen, f0);
        break;
    }

exit:

    return ret;
}


int wave_gen_set_frequency(struct wave_gen *handle, float f0)
{
    int ret = 0;

    if (!handle) {
        ret = -EINVAL;
        goto exit;
    }

    if (f0 == handle->frequency)
        goto exit;

    switch (handle->mode) {
    case WAVE_MODE_SAW:
        ret = saw_gen_set_frequency((struct saw_gen *)handle->gen, f0);
        break;
    case WAVE_MODE_SINE:
        ret = sine_gen_set_frequency((struct sine_gen *)handle->gen, f0);
        break;
    case WAVE_MODE_SQUARE:
        ret = square_gen_set_frequency((struct square_gen *)handle->gen, f0);
    default:
        break;
    }

    handle->frequency = f0;

exit:

    return ret;
}


int wave_gen_get_intensity(struct wave_gen *handle, float *intensity)
{
    int ret = 0;

    if ((!handle)
    ||  (!intensity)) {
        ret = -EINVAL;
        goto exit;
    }

    switch (handle->mode) {
    case WAVE_MODE_SAW:
        ret = saw_gen_get_intensity((struct saw_gen *)handle->gen, intensity);
        break;
    case WAVE_MODE_SINE:
        ret = sine_gen_get_intensity((struct sine_gen *)handle->gen, intensity);
        break;
    case WAVE_MODE_SQUARE:
    default:
        ret = square_gen_get_intensity((struct square_gen *)handle->gen, intensity);
        break;
    }

exit:

    return ret;
}


int wave_gen_set_intensity(struct wave_gen *handle, float intensity)
{
    int ret = 0;

    if (!handle) {
        ret = -EINVAL;
        goto exit;
    }

    switch (handle->mode) {
    case WAVE_MODE_SAW:
        ret = saw_gen_set_intensity((struct saw_gen *)handle->gen, intensity);
        break;
    case WAVE_MODE_SINE:
        ret = sine_gen_set_intensity((struct sine_gen *)handle->gen, intensity);
        break;
    case WAVE_MODE_SQUARE:
    default:
        ret = square_gen_set_intensity((struct square_gen *)handle->gen, intensity);
        break;
    }

exit:

    return ret;
}


int wave_gen_process(struct wave_gen *handle, int nb_frames, int32_t *out)
{
    int ret = 0;

    if ((!handle) || (!out)) {
        ret = -EINVAL;
        goto exit;
    }

    switch (handle->mode) {
    case WAVE_MODE_SAW:
        ret = saw_gen_process((struct saw_gen *)handle->gen, nb_frames, out);
        break;
    case WAVE_MODE_SINE:
        ret = sine_gen_process((struct sine_gen *)handle->gen, nb_frames, out);
        break;
    case WAVE_MODE_SQUARE:
    default:
        ret = square_gen_process((struct square_gen *)handle->gen, nb_frames, out);
        break;
    }

exit:

    return ret;
}
