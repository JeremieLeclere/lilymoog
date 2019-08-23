/***************************************************************************************************
 * @file moog.c
 *
 * @brief Moog like simple synthesizer module (sources)
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <moog.h>
#include <adsr.h>
#include <wave_gen.h>
#include <low_pass.h>


#define QS823_MIN       (-(1 << 23))
#define QS823_MAX       ((1 << 23) - 1)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))


struct moog {

    float fs;

    /* ADSR */
    struct adsr *adsr;
    float intensity;

    /* Low pass filter */
    struct low_pass *lpf;

    /* Internal oscillators */
    float coupling_scale;
    struct wave_gen *osc1;
    struct wave_gen *osc2;
    enum moog_osc_coupling coupling;

    /* Internal buffers */
    int frame_size;
    int32_t *osc1_output;
    int32_t *osc2_output;
    int32_t *sum_output;
    int32_t *adsr_output;
    float   *adsr_scale;
};


struct moog *moog_create(struct moog_params *params)
{
    struct moog *handle = NULL;
    struct adsr_params adsr_params;
    struct low_pass_params lpf_params;
    struct wave_gen_params osc_params;

    if (!params)
        goto failure;

    handle = (struct moog *)calloc(1, sizeof(struct moog));
    if (!handle)
        goto failure;

    handle->fs = params->fs;

    /* ADSR */
    adsr_params.fs      = params->fs;
    adsr_params.attack  = params->attack_time;
    adsr_params.decay   = params->decay_time;
    adsr_params.sustain = params->sustain;
    adsr_params.release = params->release_time;
    handle->adsr = adsr_create(&adsr_params);
    if (!handle->adsr)
        goto failure;

    /* Internal oscillators */

    handle->coupling = params->coupling;

    osc_params.fs        = params->fs;
    osc_params.f0        = 0.0;
    osc_params.intensity = 0.0;
    osc_params.mode      = params->osc_mode;
    handle->osc1 = wave_gen_create(&osc_params);
    if (!handle->osc1)
        goto failure;

    if (handle->coupling != MOOG_OSC_COUPLING_NONE) {
        handle->osc2 = wave_gen_create(&osc_params);
        if (!handle->osc2)
            goto failure;
    }

    switch (handle->coupling) {
    case MOOG_OSC_COUPLING_NONE:
        handle->coupling_scale = 0.0;
        break;
    case MOOG_OSC_COUPLING_THIRD_MINOR:
        handle->coupling_scale = pow(2, 3.0/12);
        break;
    case MOOG_OSC_COUPLING_THIRD_MAJOR:
        handle->coupling_scale = pow(2, 4.0/12);
        break;
    case MOOG_OSC_COUPLING_FIFTH:
        handle->coupling_scale = pow(2, 7.0/12);
        break;
    case MOOG_OSC_COUPLING_OCTAVE:
        handle->coupling_scale = 2.0;
        break;
    default:
        goto failure;
    }

    /* Low pass filter */
    lpf_params.Q    = params->Q;
    lpf_params.gain = params->gain;
    lpf_params.fc   = params->fc;
    lpf_params.fs   = params->fs;
    handle->lpf = low_pass_create(&lpf_params);
    if (!handle->lpf)
        goto failure;

    /* Internal buffers */
    handle->frame_size = params->frame_size;
    handle->osc1_output = (int32_t *)calloc(handle->frame_size, sizeof(int32_t));
    if (!handle->osc1_output)
        goto failure;

    if (handle->coupling != MOOG_OSC_COUPLING_NONE) {
        handle->osc2_output = (int32_t *)calloc(handle->frame_size, sizeof(int32_t));
        if (!handle->osc2_output)
            goto failure;

        handle->sum_output = (int32_t *)calloc(handle->frame_size, sizeof(int32_t));
        if (!handle->sum_output)
            goto failure;
    }

    handle->adsr_output = (int32_t *)calloc(handle->frame_size, sizeof(int32_t));
    if (!handle->adsr_output)
        goto failure;

    handle->adsr_scale = (float *)calloc(handle->frame_size, sizeof(float));
    if (!handle->adsr_scale)
        goto failure;

    return handle;

failure:

    moog_destroy(&handle);

    return NULL;
}

void moog_destroy(struct moog **handle)
{
    if ((!handle) || (!(*handle)))
        goto exit;

    adsr_destroy(&(*handle)->adsr);
    low_pass_destroy(&(*handle)->lpf);
    wave_gen_destroy(&(*handle)->osc1);
    wave_gen_destroy(&(*handle)->osc2);

    if ((*handle)->osc1_output)
        free((*handle)->osc1_output);
    if ((*handle)->osc2_output)
        free((*handle)->osc2_output);
    if ((*handle)->sum_output)
        free((*handle)->sum_output);
    if ((*handle)->adsr_output)
        free((*handle)->adsr_output);
    if ((*handle)->adsr_scale)
        free((*handle)->adsr_scale);

    free(*handle);
    *handle = NULL;

exit:

    return;
}


int moog_toggle(struct moog *handle, int state)
{
    int ret = 0;

    /* Full args check to avoid partial submodules update
     * The only remaining failure possibility lies in ADSR internal state
     * compatibility, which will be the first update attempt.
     */
    if ((!handle)
    ||  ((state != 0) && (state != 1))) {
        ret = -EINVAL;
        goto exit;
    }

    /* Update ADSR submodule */
    ret = adsr_toggle(handle->adsr, state, handle->intensity);
    if (ret)
        goto exit;

exit:

    return ret;
}


int moog_set_intensity(struct moog *handle, float intensity)
{
    int ret = 0;

    if ((!handle)
    ||  (intensity < 0)
    ||  (intensity > 1)) {
        ret = -EINVAL;
        goto exit;
    }

    handle->intensity = intensity;

    ret = wave_gen_set_intensity(handle->osc1, intensity);
    if (handle->coupling != MOOG_OSC_COUPLING_NONE)
        ret |= wave_gen_set_intensity(handle->osc2, intensity);

exit:

    return ret;
}


int moog_get_intensity(struct moog *handle, float *intensity)
{
    int ret = 0;

    if (!handle) {
        ret = -EINVAL;
        goto exit;
    }

    ret = wave_gen_get_intensity(handle->osc1, intensity);

exit:

    return ret;
}


int moog_set_frequency(struct moog *handle, float frequency)
{
    int ret = 0;

    if ((!handle)
    ||  (frequency <= 0)
    ||  (frequency >= handle->fs/2)) {
        ret = -EINVAL;
        goto exit;
    }

    ret = wave_gen_set_frequency(handle->osc1, frequency);
    if (handle->coupling != MOOG_OSC_COUPLING_NONE)
        ret |= wave_gen_set_frequency(handle->osc2, frequency * handle->coupling_scale);

exit:

    return ret;
}


int moog_get_frequency(struct moog *handle, float *frequency)
{
    int ret = 0;

    if (!handle) {
        ret = -EINVAL;
        goto exit;
    }

    ret = wave_gen_get_frequency(handle->osc1, frequency);

exit:

    return ret;
}


int moog_filter_get_parameters(struct moog *handle, float *fc, float *Q, float *gain)
{
    int ret = 0;
    struct low_pass_params params;

    if (!handle) {
        ret = -EINVAL;
        goto exit;
    }

    ret = low_pass_get_parameters(handle->lpf, &params);
    if (ret)
        goto exit;

    if (Q)
        *Q = params.Q;
    if (fc)
        *fc = params.fc;
    if (gain)
        *gain = params.gain;

exit:

    return ret;
}


int moog_filter_set_parameters(struct moog *handle, float new_fc, float new_Q, float new_gain)
{
    int ret = 0;
    struct low_pass_params new_params;

    if (!handle) {
        ret = -EINVAL;
        goto exit;
    }

    new_params.fs   = handle->fs;
    new_params.fc   = new_fc;
    new_params.Q    = new_Q;
    new_params.gain = new_gain;
    ret = low_pass_update(handle->lpf, &new_params);

exit:

    return ret;
}


int moog_filter_start_fc_sweep(struct moog *handle, float new_fc, int nb_frames)
{
    int ret = 0;

    if (!handle) {
        ret = -EINVAL;
        goto exit;
    }

    ret = low_pass_start_fc_sweep(handle->lpf, new_fc, nb_frames * handle->frame_size);

exit:

    return ret;
}


int moog_process(struct moog *handle, int32_t *output)
{
    int i, ret = 0;
    int64_t tmp_sum;

    if ((!handle)
    ||  (!output)) {
        ret = -EINVAL;
        goto exit;
    }

    /* Compute ADSR enveloppe */
    adsr_process(handle->adsr, handle->frame_size, handle->adsr_scale);

    /* Internal oscillators output */
    wave_gen_process(handle->osc1, handle->frame_size, handle->osc1_output);

    if (handle->coupling != MOOG_OSC_COUPLING_NONE) {
        wave_gen_process(handle->osc2, handle->frame_size, handle->osc2_output);

        /* Sum oscillators outputs with saturation */
        for (i = 0; i < handle->frame_size; i++) {
            tmp_sum = (int64_t)handle->osc1_output[i] + handle->osc2_output[i];
            handle->sum_output[i] = (int32_t)(MAX(MIN(tmp_sum, QS823_MAX), QS823_MIN));
        }

        /* Apply ADSR enveloppe on summed oscillators outputs */
        for (i = 0; i < handle->frame_size; i++)
            handle->adsr_output[i] = (int32_t)(handle->adsr_scale[i] * handle->sum_output[i]);

    } else {

        /* Apply ADSR enveloppe on single oscillator output */
        for (i = 0; i < handle->frame_size; i++)
            handle->adsr_output[i] = (int32_t)(handle->adsr_scale[i] * handle->osc1_output[i]);
    }

    /* Low pass filter */
    low_pass_process(handle->lpf, handle->adsr_output, handle->frame_size, output);

exit:

    return ret;
}
