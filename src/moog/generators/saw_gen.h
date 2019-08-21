/***************************************************************************************************
 * @file saw_gen.h
 *
 * @brief Saw waveform generation module (header)
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

#ifndef _SAW_GEN_H_
#define _SAW_GEN_H_

#include <errno.h>
#include <stdint.h>


/**
 * @brief Opaque module handle
 */
struct saw_gen;


/**
 * @brief Initialization parameters
 */
struct saw_gen_params {
    float fs;                       ///< Sampling frequency (Hz, > 0)
    float f0;                       ///< Saw waveform frequency (Hz, [0,fs/2[)
    float intensity;                ///< Saw waveform intensity ([0,1])
};


/**
 * @brief Initialize saw waveform generator module
 *
 * @param[in] params    : Initialization parameters
 *
 * @return Module handle if successful, NULL else
 */
struct saw_gen *saw_gen_create(struct saw_gen_params *params);


/**
 * @brief Release module ressources
 *
 * @param[in] handle    : Module handle
 *
 * @return None
 */
void saw_gen_destroy(struct saw_gen **handle);


/**
 * @brief Get current saw waveform frequency
 *
 * @param[in]  handle   : Module handle
 * @param[out] f0       : Current frequency (Hz)
 *
 * @return 0 if successful, 0 > errno else
 */
int saw_gen_get_frequency(struct saw_gen *handle, float *f0);


/**
 * @brief Update saw waveform frequency
 *
 * @param[in] handle    : Module handle
 * @param[in] f0        : New frequency (Hz, [0,fs/2[)
 *
 * @return 0 if successful, 0 > errno else
 */
int saw_gen_set_frequency(struct saw_gen *handle, float f0);


/**
 * @brief Get current saw waveform intensity
 *
 * @param[in]  handle       : Module handle
 * @param[out] intensity    : Current intensity
 *
 * @return 0 if successful, 0 > errno else
 */
int saw_gen_get_intensity(struct saw_gen *handle, float *intensity);


/**
 * @brief Update saw waveform intensity
 *
 * @param[in] handle    : Module handle
 * @param[in] intensity : New intensity ([0,1])
 *
 * @return 0 if successful, 0 > errno else
 */
int saw_gen_set_intensity(struct saw_gen *handle, float intensity);


/**
 * @brief Proceed to saw waveform generation
 *
 * @param[in]  handle       : Module handle
 * @param[in]  nb_frames    : Number of output frames
 * @param[out] out          : Output QS8.23 frames
 *
 * @return 0 if successful, 0 > errno else
 */
int saw_gen_process(struct saw_gen *handle, int nb_frames, int32_t *out);


#endif /* _SAW_GEN_H_ */
