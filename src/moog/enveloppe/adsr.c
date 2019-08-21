/***************************************************************************************************
 * @file adsr.c
 *
 * @brief Attack/Decay/Sustain/Release module (sources)
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

#include <adsr.h>


/* Internal ADSR states */
enum adsr_state {
    ADSR_IDLE,                          ///< No note on going
    ADSR_ATTACK,                        ///< Attack slope
    ADSR_DECAY,                         ///< Decay slope
    ADSR_SUSTAIN,                       ///< Sustain mode
    ADSR_RELEASE                        ///< Release slope
};


struct adsr {

    float sustain;                      ///< Sustain factor (in [0, 1], applied to intensity)
    float intensity;                    ///< User defined intensity

    /* Decay slope */
    int decay_len;
    float *decay_table;

    /* Attack slope */
    int attack_len;
    float *attack_table;

    /* Release slope */
    int release_len;
    float *release_table;

    /* State description */
    int index;
    float state_factor;
    enum adsr_state state;
};


static void adsr_state_update(struct adsr *handle)
{
    switch (handle->state) {

    case ADSR_IDLE:
        /* No on going note, just chill ... */
        break;

    case ADSR_ATTACK:

        handle->index++;
        if (handle->index < handle->attack_len) {

            /* Keep following attack slope */
            handle->state_factor = handle->attack_table[handle->index];

        } else {

            /* Switch to decay slope */
            handle->state        = ADSR_DECAY;
            handle->index        = 0;
            handle->state_factor = handle->decay_table[handle->index];
        }
        break;

    case ADSR_DECAY:

        handle->index++;
        if (handle->index < handle->decay_len) {

            /* Keep following decay slope */
            handle->state_factor = handle->decay_table[handle->index];

        } else {

            /* Switch to sustain mode */
            handle->state        = ADSR_SUSTAIN;
            handle->index        = 0;
            handle->state_factor = handle->sustain;
        }
        break;

    case ADSR_SUSTAIN:
        /* Someone has a finger stuck on the keyboard, just wait and don't break the groove ! */
        break;

    case ADSR_RELEASE:

        handle->index++;
        if (handle->index < handle->release_len) {

            /* Keep following release slope */
            handle->state_factor = handle->release_table[handle->index];

        } else {

            /* Switch to idle mode */
            handle->state        = ADSR_IDLE;
            handle->index        = 0;
            handle->intensity    = 0.0;
            handle->state_factor = 0.0;
        }
        break;
    }
}


struct adsr *adsr_create(struct adsr_params *params)
{
    int i;
    struct adsr *handle = NULL;

    if ((!params)
    ||  (params->fs <= 0)
    ||  (params->attack <= 0)
    ||  (params->decay <= 0)
    ||  (params->sustain <= 0)
    ||  (params->sustain > 1)
    ||  (params->release < 0))
        goto failure;

    handle = (struct adsr *)calloc(1, sizeof(struct adsr));
    if (!handle)
        goto failure;

    handle->index        = 0;
    handle->state        = ADSR_IDLE;
    handle->sustain      = params->sustain;
    handle->intensity    = 0.0;
    handle->state_factor = 0.0;

    handle->attack_len   = (int)(params->attack * params->fs / 1000);
    handle->attack_table = (float *)calloc(handle->attack_len, sizeof(float));
    if (!handle->attack_table)
        goto failure;
    for (i = 0; i < handle->attack_len; i++)
        handle->attack_table[i] = (float)i / handle->attack_len;


    handle->decay_len    = (int)(params->decay * params->fs / 1000);
    handle->decay_table = (float *)calloc(handle->decay_len, sizeof(float));
    if (!handle->decay_table)
        goto failure;
    for (i = 0; i < handle->decay_len; i++)
        handle->decay_table[i] = handle->sustain + (1 - handle->sustain)
                                 * (handle->decay_len - i) / handle->decay_len;

    handle->release_len  = (int)(params->release * params->fs / 1000);
    handle->release_table = (float *)calloc(handle->release_len, sizeof(float));
    if (!handle->release_table)
        goto failure;
    for (i = 0; i < handle->release_len; i++)
        handle->release_table[i] = handle->sustain
                                   * (handle->release_len - i) / handle->release_len;

    handle->state = ADSR_IDLE;

    return handle;

failure:

    adsr_destroy(&handle);

    return NULL;
}


void adsr_destroy(struct adsr **handle)
{
    if ((!handle) || (!(*handle)))
        goto exit;

    if ((*handle)->attack_table)
        free((*handle)->attack_table);

    if ((*handle)->decay_table)
        free((*handle)->decay_table);

    if ((*handle)->release_table)
        free((*handle)->release_table);

    free(*handle);
    *handle = NULL;

exit:

    return;
}


int adsr_toggle(struct adsr *handle, int state, float intensity)
{
    int ret = 0;

    if ((!handle)
    || (intensity < 0)
    || (intensity > 1)
    || ((state != 0) && (state != 1))) {
        ret = -EINVAL;
        goto exit;
    }

    if (state == 1) {

        /* Avoid note switch during transitions (shall be fixed ...) */
        if ((handle->state != ADSR_IDLE)
        &&  (handle->state != ADSR_SUSTAIN)) {
            ret = -EAGAIN;
        } else {
            if (handle->state == ADSR_IDLE) {
                handle->intensity    = intensity;
                handle->state        = ADSR_ATTACK;
                handle->index        = 0;
                handle->state_factor = 0.0;
            }
            /* No else: If we're in sustain mode, just stay like this */
        }

    } else {

        if ((handle->state == ADSR_IDLE)
        ||  (handle->state == ADSR_RELEASE)) {
            /* we're already in release or idle mode, silently conitnue */
            ret = 0;
        } else {
            /* Switch to release mode, whatever current state.
             * In case we're in attack or decay state, that will generate a state_factor
             * discontinuity ... To be reworked, somewhere, sometime.
             */
            handle->state        = ADSR_RELEASE;
            handle->index        = 0;
            handle->state_factor = handle->sustain;
        }
    }

exit:

    return ret;
}


int adsr_process(struct adsr *handle, int nb_frames, float *enveloppe)
{
    int i, ret = 0;

    if ((!handle)
    || (!enveloppe)) {
        ret = -EINVAL;
        goto exit;
    }

    for (i = 0; i < nb_frames; i++) {

        enveloppe[i] = handle->intensity * handle->state_factor;

        adsr_state_update(handle);
    }

exit:

    return ret;
}
