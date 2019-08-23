/***************************************************************************************************
 * @file low_pass.c
 *
 * @brief Low pass filter module (sources)
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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <low_pass.h>


/* Double to QS8.23 conversion macros */
#define BQ_DOUBLE_2_QS328(val)  (int32_t)((val)*(1 << (28)) + (((val) > 0)? 0.5: -0.5))

/* Transition table: Q.16 values representing a [0,1[ linear table */
#define TABLE_LEN       (256)
#define TABLE_SCALE     (16)

static uint16_t transition_table[TABLE_LEN] = {
        0,   256,   512,   768,  1024,  1280,  1536,  1792,
     2048,  2304,  2560,  2816,  3072,  3328,  3584,  3840,
     4096,  4352,  4608,  4864,  5120,  5376,  5632,  5888,
     6144,  6400,  6656,  6912,  7168,  7424,  7680,  7936,
     8192,  8448,  8704,  8960,  9216,  9472,  9728,  9984,
    10240, 10496, 10752, 11008, 11264, 11520, 11776, 12032,
    12288, 12544, 12800, 13056, 13312, 13568, 13824, 14080,
    14336, 14592, 14848, 15104, 15360, 15616, 15872, 16128,
    16384, 16640, 16896, 17152, 17408, 17664, 17920, 18176,
    18432, 18688, 18944, 19200, 19456, 19712, 19968, 20224,
    20480, 20736, 20992, 21248, 21504, 21760, 22016, 22272,
    22528, 22784, 23040, 23296, 23552, 23808, 24064, 24320,
    24576, 24832, 25088, 25344, 25600, 25856, 26112, 26368,
    26624, 26880, 27136, 27392, 27648, 27904, 28160, 28416,
    28672, 28928, 29184, 29440, 29696, 29952, 30208, 30464,
    30720, 30976, 31232, 31488, 31744, 32000, 32256, 32512,
    32768, 33024, 33280, 33536, 33792, 34048, 34304, 34560,
    34816, 35072, 35328, 35584, 35840, 36096, 36352, 36608,
    36864, 37120, 37376, 37632, 37888, 38144, 38400, 38656,
    38912, 39168, 39424, 39680, 39936, 40192, 40448, 40704,
    40960, 41216, 41472, 41728, 41984, 42240, 42496, 42752,
    43008, 43264, 43520, 43776, 44032, 44288, 44544, 44800,
    45056, 45312, 45568, 45824, 46080, 46336, 46592, 46848,
    47104, 47360, 47616, 47872, 48128, 48384, 48640, 48896,
    49152, 49408, 49664, 49920, 50176, 50432, 50688, 50944,
    51200, 51456, 51712, 51968, 52224, 52480, 52736, 52992,
    53248, 53504, 53760, 54016, 54272, 54528, 54784, 55040,
    55296, 55552, 55808, 56064, 56320, 56576, 56832, 57088,
    57344, 57600, 57856, 58112, 58368, 58624, 58880, 59136,
    59392, 59648, 59904, 60160, 60416, 60672, 60928, 61184,
    61440, 61696, 61952, 62208, 62464, 62720, 62976, 63232,
    63488, 63744, 64000, 64256, 64512, 64768, 65024, 65280
};


/* Biquad normalized coefficients (floating point) */
struct low_pass_coeffs {
    double b0;                                          ///< Might ... No, should look familiar:
    double b1;                                          ///<
    double b2;                                          ///< y[n] = b0.x[n] + b1.x[n-1] + b2.x[n-2]
    double a1;                                          ///<        - a1.y[n-1] - a2.y[n-2]
    double a2;                                          ///<
};


/* Biquad normalized coefficients (fixed point) */
struct low_pass_fp_coeffs {
    int32_t b0;                                         ///< Might ... No, should look familiar:
    int32_t b1;                                         ///<
    int32_t b2;                                         ///< y[n] = b0.x[n] + b1.x[n-1] + b2.x[n-2]
    int32_t a1;                                         ///<        - a1.y[n-1] - a2.y[n-2]
    int32_t a2;                                         ///<
};


/* Low pass structure */
struct low_pass {
    int32_t x1;
    int32_t x2;
    int32_t y1;
    int32_t y2;
    float sweep_fc;
    int sweep_flag;
    int sweep_index;
    int sweep_length;
    float sweep_step;
    int table_index;
    int update_flag;
    struct low_pass_fp_coeffs coeffs;
    struct low_pass_params parameters;
    struct low_pass_fp_coeffs new_coeffs;
};


/* Compute filter coefficients from user parameters */
static int low_pass_design(const struct low_pass_params *params, struct low_pass_coeffs *coeffs)
{
    int ret = 0;
    double a0, k;

    if ((!params) || (!coeffs)) {
        ret = -EINVAL;
        goto exit;
    }

    /* Further used as denominator */
    if ((params->Q <= 0)
    ||  (params->fs <= 0)
    ||  (params->fc >= (params->fs)/2)
    ||  (params->fc <= 0)) {
        ret = -EINVAL;
        goto exit;
    }

    memset(coeffs, 0, sizeof(struct low_pass_coeffs));

    k  = tan(M_PI * params->fc / params->fs);
    a0 = params->Q + k + params->Q * k * k;

    coeffs->b0 = k * k * params->Q / a0;
    coeffs->b1 = 2 * k * k * params->Q / a0;
    coeffs->b2 = k * k * params->Q / a0;
    coeffs->a1 = 2 * params->Q * (k * k - 1) / a0;
    coeffs->a2 = (params->Q - k + k * k * params->Q) / a0;

exit:

    return ret;
}


/* Fill current/upcoming fixed point coefficients structure */
static void low_pass_feed(struct low_pass *handle, const struct low_pass_coeffs *coeffs, int update)
{
    if (update) {

        handle->new_coeffs.b0 = BQ_DOUBLE_2_QS328(coeffs->b0);
        handle->new_coeffs.b1 = BQ_DOUBLE_2_QS328(coeffs->b1);
        handle->new_coeffs.b2 = BQ_DOUBLE_2_QS328(coeffs->b2);
        handle->new_coeffs.a1 = BQ_DOUBLE_2_QS328(coeffs->a1);
        handle->new_coeffs.a2 = BQ_DOUBLE_2_QS328(coeffs->a2);

    } else {

        handle->coeffs.b0 = BQ_DOUBLE_2_QS328(coeffs->b0);
        handle->coeffs.b1 = BQ_DOUBLE_2_QS328(coeffs->b1);
        handle->coeffs.b2 = BQ_DOUBLE_2_QS328(coeffs->b2);
        handle->coeffs.a1 = BQ_DOUBLE_2_QS328(coeffs->a1);
        handle->coeffs.a2 = BQ_DOUBLE_2_QS328(coeffs->a2);

    }
}


/* Sweep coefficients update */
int low_pass_sweep_update(struct low_pass *handle)
{
    int ret = 0;
    struct low_pass_params new_params;
    struct low_pass_coeffs new_coeffs;

    /* Set new parameters */
    memcpy(&new_params, &handle->parameters, sizeof(struct low_pass_params));

    handle->sweep_index++;
    if (handle->sweep_index == handle->sweep_length) {
        /* Set cutoff frequency to target and exit sweep mode */
        new_params.fc = handle->sweep_fc;
        handle->sweep_flag = 0;
    } else {
        new_params.fc += handle->sweep_step;
    }

    /* Convert user parameters to internal coefficients */
    ret = low_pass_design(&new_params, &new_coeffs);
    if (ret)
        goto exit;

    /* Update biquad upcoming coefficients */
    low_pass_feed(handle, &new_coeffs, 1);

    handle->table_index  = 0;
    handle->update_flag  = 1;

    /* Save new parameters */
    memcpy(&handle->parameters, &new_params, sizeof(struct low_pass_params));

exit:

    return ret;
}


/* Filter coefficients linear progression to target values */
static void low_pass_update_coeffs(struct low_pass *handle)
{
    int32_t delta;
    uint16_t scale;

    /* Update filter coefficients (simple linear interpolation) */
    scale = transition_table[handle->table_index];
    delta = handle->new_coeffs.b0 - handle->coeffs.b0;
    handle->coeffs.b0 += (int32_t)(((int64_t)delta * scale) >> TABLE_SCALE);
    delta = handle->new_coeffs.b1 - handle->coeffs.b1;
    handle->coeffs.b1 += (int32_t)(((int64_t)delta * scale) >> TABLE_SCALE);
    delta = handle->new_coeffs.b2 - handle->coeffs.b2;
    handle->coeffs.b2 += (int32_t)(((int64_t)delta * scale) >> TABLE_SCALE);
    delta = handle->new_coeffs.a1 - handle->coeffs.a1;
    handle->coeffs.a1 += (int32_t)(((int64_t)delta * scale) >> TABLE_SCALE);
    delta = handle->new_coeffs.a2 - handle->coeffs.a2;
    handle->coeffs.a2 += (int32_t)(((int64_t)delta * scale) >> TABLE_SCALE);

    /* Update transition descriptors */
    handle->table_index++;
    if (handle->table_index == TABLE_LEN) {
        /* Current transition is over */
        memcpy(&handle->coeffs, &handle->new_coeffs, sizeof(struct low_pass_fp_coeffs));
        handle->update_flag = 0;

        /* Update sweep state */
        if (handle->sweep_flag)
            low_pass_sweep_update(handle);

    }
}


struct low_pass *low_pass_create(const struct low_pass_params *params)
{
    struct low_pass_coeffs coeffs;
    struct low_pass *handle = NULL;

    if (!params)
        goto failure;

    handle = (struct low_pass*)calloc(1, sizeof(struct low_pass));
    if (!handle)
        goto failure;

    if (low_pass_design(params, &coeffs))
        goto failure;

    low_pass_feed(handle, &coeffs, 0);

    memcpy(&handle->parameters, params, sizeof(struct low_pass_params));

    return handle;

failure:

    low_pass_destroy(&handle);

    return NULL;
}


void low_pass_destroy(struct low_pass **handle)
{
    if ((!handle) || (!(*handle)))
        goto exit;

    free(*handle);
    *handle = NULL;

exit:

    return;
}


int low_pass_update(struct low_pass *handle, const struct low_pass_params *new_params)
{
    int ret = 0;
    struct low_pass_coeffs new_coeffs;

    if ((!handle) || (!new_params)) {
        ret = -EINVAL;
        goto exit;
    }

    /* Prevent update during sweep */
    if (handle->sweep_flag) {
        ret = -EAGAIN;
        goto exit;
    }

    /* Convert user parameters to internal coefficients */
    ret = low_pass_design(new_params, &new_coeffs);
    if (ret)
        goto exit;

    handle->table_index  = 0;
    handle->update_flag  = 1;

    /* update biquad upcoming coefficients */
    low_pass_feed(handle, &new_coeffs, 1);

    memcpy(&handle->parameters, new_params, sizeof(struct low_pass_params));

exit:

    return ret;
}


int low_pass_get_parameters(struct low_pass *handle, struct low_pass_params *params)
{
    int ret = 0;

    if ((!handle) || (!params)) {
        ret = -EINVAL;
        goto exit;
    }

    memcpy(params, &handle->parameters, sizeof(struct low_pass_params));

exit:

    return ret;
}


int low_pass_start_fc_sweep(struct low_pass *handle, float new_fc, int nb_samples)
{
    int ret = 0;

    if ((!handle)
    ||  (new_fc <= 0)
    ||  (new_fc >= handle->parameters.fs/2)
    ||  (nb_samples <= 0)) {
        ret = -EINVAL;
        goto exit;
    }

    /* Check if a sweep is already on going */
    if (handle->sweep_flag) {
        ret = -EAGAIN;
        goto exit;
    }

    handle->sweep_fc     = new_fc;
    handle->sweep_flag   = 1;
    handle->sweep_index  = 0;
    handle->sweep_length = nb_samples / TABLE_LEN;
    handle->sweep_step   = (new_fc - handle->parameters.fc) / handle->sweep_length;

    low_pass_sweep_update(handle);

exit:

    return ret;
}


int low_pass_process(struct low_pass *handle, const int32_t *in, int nb_frames, int32_t *out)
{
    int ret = 0;
    int64_t acc;
    int32_t *output = out;
    const int32_t *input = in;

    if ((!handle) || (!in) || (!out) || (nb_frames <= 0)) {
        ret = -EINVAL;
        goto exit;
    }

    while (nb_frames--) {

        /* Handle transition if any */
        if (handle->update_flag)
            low_pass_update_coeffs(handle);

        acc = (int64_t)handle->coeffs.b0 * (*input)
            + (int64_t)handle->coeffs.b1 * handle->x1
            + (int64_t)handle->coeffs.b2 * handle->x2
            - (int64_t)handle->coeffs.a1 * handle->y1
            - (int64_t)handle->coeffs.a2 * handle->y2;

        /* Update states */
        handle->x2 = handle->x1;
        handle->x1 = *input;
        handle->y2 = handle->y1;

        if (acc < 0)
            acc += (1 << 28) - 1;
        *output = handle->y1 = (int32_t)(acc >> 28);

        output++;
        input++;
    }

exit:

    return ret;
}
