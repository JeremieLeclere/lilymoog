/***************************************************************************************************
 * @file low_pass.h
 *
 * @brief Low pass filter module (header)
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

#ifndef _LOW_PASS_H_
#define _LOW_PASS_H_


#include <errno.h>
#include <stdint.h>


/**
 * @brief Opaque module handle
 */
struct low_pass;


/**
 * @brief Initialization parameters
 */
struct low_pass_params {
    float Q;                            ///< Quality factor (> 0)
    float gain;                         ///< Gain (in dB)
    float fc;                           ///< Center frequency (Hz, < fs/2)
    float fs;                           ///< Sampling frequency (Hz)
};


/**
 * @brief Low pass module creation
 *
 * @param[in] params        : Filter initialization parameters
 *
 * @return Valid module handle if successful, NULL else
 */
struct low_pass *low_pass_create(const struct low_pass_params *params);


/**
 * @brief Release module ressources
 *
 * @param[in] handle        : Module handle
 *
 * @return None
 */
void low_pass_destroy(struct low_pass **handle);


/**
 * @brief Update filter parameters
 *
 * @param[in] handle        : Module handle
 * @param[in] new_params    : New filter parameters
 *
 * @return 0 if successful, 0 > errno else
 */
int low_pass_update(struct low_pass *handle, const struct low_pass_params *new_params);


/**
 * @brief Get filter parameters
 *
 * @param[in]  handle       : Module handle
 * @param[out] params       : filter parameters
 *
 * @return 0 if successful, 0 > errno else
 */
int low_pass_get_parameters(struct low_pass *handle, struct low_pass_params *params);


/**
 * @brief Proceed to low pass filtering
 *
 * @param[in]  handle       : Module handle
 * @param[in]  in           : Input QS8.23 samples
 * @param[in]  nb_frames    : Number of frames
 * @param[out] out          : Output QS8.23 samples
 *
 * @return 0 if successful, 0 > errno else
 */
int low_pass_process(struct low_pass *handle, const int32_t *in, int nb_frames, int32_t *out);


#endif /* _LOW_PASS_H_ */
