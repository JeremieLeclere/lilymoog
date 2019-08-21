/***************************************************************************************************
 * @file moog.h
 *
 * @brief Moog like simple synthesizer module (header)
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

#ifndef _MOOG_H_
#define _MOOG_H_


#include <errno.h>
#include <stdint.h>

/* Forward waveform types enum declaration */
#include <wave_gen.h>


/**
 * @brief Opaque module handle
 */
struct moog;


/**
 * @brief Internal oscillator coupling mode
 *
 *  The values defined in this enum are used to set the interval between the
 * first oscillator's frequency (user provided) and the second oscillator's
 * frequency.
 */
enum moog_osc_coupling {
    MOOG_OSC_COUPLING_NONE,                     ///< Use a single oscillator
    MOOG_OSC_COUPLING_THIRD_MINOR,              ///< Minor third interval
    MOOG_OSC_COUPLING_THIRD_MAJOR,              ///< Major third interval
    MOOG_OSC_COUPLING_FIFTH,                    ///< Fifth interval
    MOOG_OSC_COUPLING_OCTAVE                    ///< Let's double that frequency !
};

/**
 * @brief Initialization parameters
 */
struct moog_params {

    /* General parameters */
    float fs;                                   ///< Sampling frequency
    int frame_size;                             ///< Numer of samples per frame

    /* Low pass filter parameters */
    float fc;                                   ///< Cutoff frequency (Hz, [0,fs/2[)
    float Q;                                    ///< Quality factor (> 0)
    float gain;                                 ///< Gain (dB)

    /* ADSR parameters */
    float attack_time;                          ///< Attack time (ms, > 0)
    float decay_time;                           ///< Decay time (ms, > 0)
    float sustain;                              ///< Sustain factor ([0,1])
    float release_time;                         ///< Release time (ms, > 0)

    /* Oscillator parameters */
    enum wave_gen_mode osc_mode;                ///< Waveform type
    enum moog_osc_coupling coupling;            ///< Oscillators coupling mode
};


/**
 * @brief Initialize moog bass module
 *
 * @param[in] params        : Initialization parameters
 *
 * @return Module handle if successful, NULL else
 */
struct moog *moog_create(struct moog_params *params);


/**
 * @brief Release module ressources
 *
 * @param[in] handle        : Module handle
 *
 * @return None
 */
void moog_destroy(struct moog **handle);


/**
 * @brief Toggle Moog module (ON/OFF)
 *
 * @param[in] handle        : Module handle
 * @param[in] state         : ON (1), OFF (0)
 *
 * @return 0 if successful, 0 > errno else
 */
int moog_toggle(struct moog *handle, int state);


/**
 * @brief Set output intensity
 *
 * @param[in] handle        : Module handle
 * @param[in] intensity     : Note intensity (in [0,1], with max(QS8.23) as level 1)
 *
 * @return 0 if successful, 0 > errno else
 */
int moog_set_intensity(struct moog *handle, float intensity);


/**
 * @brief Get output intensity
 *
 * @param[in]  handle       : Module handle
 * @param[out] intensity    : Output intensity  (in [0,1], with max(QS8.23) as level 1)
 *
 * @return 0 if successful, 0 > errno else
 */
int moog_get_intensity(struct moog *handle, float *intensity);


/**
 * @brief Set low oscillator output frequency
 *
 * @param[in] handle        : Module handle
 * @param[in] f0            : Note frequency
 *
 * @return 0 if successful, 0 > errno else
 */
int moog_set_frequency(struct moog *handle, float frequency);


/**
 * @brief Get low oscillator output frequency
 *
 * @param[în]  handle       : Module handle
 * @param[out] frequency    : Low oscillator frequency
 */
int moog_get_frequency(struct moog *handle, float *frequency);


/**
 * @brief Update low pass filter properties
 *
 * @param[in] handle        : Module handle
 * @param[in] fc            : New cutoff frequency (Hz, [0,fs/2[)
 * @param[in] Q             : Quality factor
 * @param[in] gain          : Gain (dB)
 *
 * @return 0 if successful, 0 > errno else
 */
int moog_filter_update(struct moog *handle, float fc, float Q, float gain);


/**
 * @brief Proceed to moog bass generation
 *
 * @param[in]  handle       : Module handle
 * @param[in]  nb_frames    : Numer of output samples
 * @param[out] output       : Output QS8.23 signal
 *
 * @return 0 if successful, 0 > errno
 */
int moog_process(struct moog *handle, int32_t *output);


#endif /* _MOOG_H_ */
