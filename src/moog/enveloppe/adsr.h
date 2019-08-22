/***************************************************************************************************
 * @file adsr.h
 *
 * @brief Attack/Decay/Sustain/Release module (header)
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

#ifndef _ADSR_H_
#define _ADSR_H_

#include <errno.h>
#include <stdint.h>


/**
 * @brief Opaque module handle
 */
struct adsr;


/**
 * @brief Initialization parameters
 */
struct adsr_params {
    float fs;                               ///< Sampling frequency (Hz, > 0)
    float attack;                           ///< Attack time (ms, > 0)
    float decay;                            ///< Decay time (ms, > 0)
    float sustain;                          ///< Sustain factor (no unit, [0,1])
    float release;                          ///< Release time (ms, > 0)
};


/**
 * @brief Initialize ADSR module
 *
 * @param[in] params    : Initialization parameters
 *
 * @return Module handle if successful, NULL else
 */
struct adsr *adsr_create(struct adsr_params *params);


/**
 * @brief Release ressources
 *
 * @param[in] handle    : Module handle
 *
 * @return None
 */
void adsr_destroy(struct adsr **handle);


/**
 * @brief Toggle note (ON/OFF)
 *
 * @param[in] handle    : Module handle
 * @param[in] state     : ON (1), OFF (0)
 * @param[in] intensity : Note intensity (only used if state == ON)
 *
 * @return 0 if successful, 0 > errno else
 */
int adsr_toggle(struct adsr *handle, int state, float intensity);


/**
 * @brief Proceed to enveloppe computation
 *
 * @param[in]  handle       : Module handle
 * @param[in]  nb_frames    : Numer of samples
 * @param[out] enveloppe    : Output enveloppe
 *
 * @return 0 if successful, 0 > errno else
 */
int adsr_process(struct adsr *handle, int nb_frames, float *enveloppe);


#endif /* _ADSR_H_ */
