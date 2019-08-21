/***************************************************************************************************
 * @file sine_gen.c
 *
 * @brief Sinus waveform generation module (sources)
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
#include <string.h>
#include <stdlib.h>

#include <sine_gen.h>


#define ABS(x)                      (((x) > 0) ? (x) : -(x))
#define MIN(x,y)                    (((x) < (y)) ? (x) : (y))
#define SIGN(x)                     (((x) >= 0) ? 1 : -1)
#define QS823_MAX                   ((1 << 23) - 1)                 ///< QS8.23 scale factor
#define INTENSITY_TRANSITION_LEN    (1000)
#define FREQUENCY_TRANSITION_LEN    (256)


struct sine_gen {

    /* General parameters */
    float fs;
    float f0;
    float phase;
    uint64_t index;
    float intensity;

    /* Amplitude transition descriptors */
    float intensity_delta;
    int intensity_transition;
    int intensity_transition_index;

    /* Trackers for frequency transition */
    int ascending;
    int prev_sign;
    int sign_change;
    int32_t prev_out;

    /* Frequency transition descriptors */
    float new_f0;
    int frequency_transition;
    int frequency_transition_start;
    int frequency_transition_index;
    float frequency_transition_phase[FREQUENCY_TRANSITION_LEN];
};


struct sine_gen *sine_gen_create(struct sine_gen_params *params)
{
    struct sine_gen *handle = NULL;

    if ((!params)
    ||  (params->fs < 0)
    ||  (params->f0 < 0)
    ||  (params->f0 >= params->fs/2)
    ||  (params->intensity < 0)
    ||  (params->intensity > 1))
        goto failure;

    handle = (struct sine_gen *)calloc(1, sizeof(struct sine_gen));
    if (!handle)
        goto failure;

    handle->f0        = params->f0;
    handle->fs        = params->fs;
    handle->intensity = params->intensity;
    handle->index     = 0;

    return handle;

failure:

    sine_gen_destroy(&handle);

    return NULL;
}


void sine_gen_destroy(struct sine_gen **handle)
{
    if ((!handle) || (!(*handle)))
        goto exit;

    free(*handle);
    *handle = NULL;

exit:

    return;
}

int sine_gen_get_frequency(struct sine_gen *handle, float *f0)
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


int sine_gen_set_frequency(struct sine_gen *handle, float f0)
{
    int i, ret = 0;
    float f, delta;

    if ((!handle)
    ||  (f0 < 0)
    ||  (f0 >= handle->fs/2)) {
        ret = -EINVAL;
        goto exit;
    }

    if (handle->f0 == 0) {
        handle->f0 = f0;
        goto exit;
    }

    if (handle->frequency_transition) {
        ret = -EAGAIN;
        goto exit;
    }

    delta = (f0 - handle->f0) / FREQUENCY_TRANSITION_LEN;
    handle->frequency_transition_phase[0] = handle->f0;
    for (i = 1; i < FREQUENCY_TRANSITION_LEN; i++) {
        f = handle->f0 + i*delta;
        handle->frequency_transition_phase[i] = handle->frequency_transition_phase[i - 1] + f;
    }

    handle->new_f0                     = f0;
    handle->frequency_transition       = 1;
    handle->frequency_transition_start = 0;
    handle->frequency_transition_index = 0;

exit:

    return ret;
}


int sine_gen_get_intensity(struct sine_gen *handle, float *intensity)
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


int sine_gen_set_intensity(struct sine_gen *handle, float intensity)
{
    int ret = 0;

    if ((!handle)
    ||  (intensity < 0)
    ||  (intensity > 1)) {
        ret = -EINVAL;
        goto exit;
    }

    handle->intensity_transition       = 1;
    handle->intensity_delta            = (intensity - handle->intensity) / INTENSITY_TRANSITION_LEN;
    handle->intensity_transition_index = 0;

exit:

    return ret;
}


int sine_gen_process(struct sine_gen *handle, int nb_frames, int32_t *out)
{
    float phase;
    int period, index = 0;
    int i, j, sign, ret = 0;
    int32_t delta, delta_min = QS823_MAX;
    int32_t tmp, prev_tmp, local_ascending = 0;

    if ((!handle) || (!out)) {
        ret = -EINVAL;
        goto exit;
    }

    for (i = 0; i < nb_frames; i++) {

        /* Handle amplitude transition */
        if (handle->intensity_transition) {
            handle->intensity += handle->intensity_delta;
            handle->intensity_transition_index++;
            if (handle->intensity_transition_index == INTENSITY_TRANSITION_LEN) {
                handle->intensity_transition       = 0;
                handle->intensity_delta            = 0;
                handle->intensity_transition_index = 0;
            }
        }

        /* Handle frequency transition */
        if (handle->frequency_transition) {

            /* Wait for an ascending sign change to start */
            if ((!handle->frequency_transition_start)
            &&  (handle->sign_change)
            &&  (handle->ascending))
                handle->frequency_transition_start = 1;

            if (handle->frequency_transition_start) {

                /* Proceed to frequency transition */
                phase = handle->frequency_transition_phase[handle->frequency_transition_index];
                handle->frequency_transition_index++;

                /* Handle end of frequency transition */
                if (handle->frequency_transition_index == FREQUENCY_TRANSITION_LEN) {

                    /* Reset descriptors */
                    handle->f0                         = handle->new_f0;
                    handle->new_f0                     = 0;
                    handle->frequency_transition       = 0;
                    handle->frequency_transition_index = 0;
                    handle->frequency_transition_start = 0;
                    memset(handle->frequency_transition_phase,
                           0,
                           FREQUENCY_TRANSITION_LEN * sizeof(float));

                    /* Search for best new index */
                    period = handle->fs / handle->f0;
                    for (j = 0; j < period; j++) {

                        /* Compute output for current index */
                        tmp = (int32_t)(QS823_MAX * handle->intensity
                                        * sin(2 * M_PI * j * handle->f0 / handle->fs));

                        /* Sine ascending status */
                        if (j == 0)
                            prev_tmp = tmp;
                        else {
                            if (tmp > prev_tmp)
                                local_ascending = 1;
                            else
                                local_ascending = 0;
                            prev_tmp = tmp;
                        }

                        /* Check for minimal delta between last output and computed value, only
                         * if we're in the expected sine phase (else we might find an non relevant
                         * minimal delta that will ruin that search)
                         */
                        if (((handle->ascending) && (local_ascending))
                        ||  ((!handle->ascending) && (!local_ascending))) {
                            delta = ABS(tmp - handle->prev_out);
                            if (delta < delta_min) {
                                delta_min = delta;
                                /* Store best candidate index */
                                index = j;
                            }
                        }
                    }
                    handle->index = index + 1;
                }

            } else {
                phase = handle->index * handle->f0;
                handle->index++;
            }

        } else {
            phase = handle->index * handle->f0;
            handle->index++;
        }

        out[i] = (int32_t)(QS823_MAX * handle->intensity * sin(2 * M_PI * phase / handle->fs));

        /* Update trackers */
        sign = SIGN(out[i]);
        if (sign != handle->prev_sign) {
            handle->prev_sign   = sign;
            handle->sign_change = 1;
        } else {
            handle->sign_change = 0;
        }
        if (out[i] > handle->prev_out)
            handle->ascending = 1;
        else
            handle->ascending = 0;

        handle->prev_out = out[i];
    }

exit:

    return ret;
}
