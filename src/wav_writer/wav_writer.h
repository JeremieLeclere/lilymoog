/***************************************************************************************************
 * @file wav_writer.h
 *
 * @brief WAV file writer module (header)
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

#ifndef _WAV_WRITER_H_
#define _WAV_WRITER_H_


#include <errno.h>
#include <stdint.h>


/**
 * @brief Opaque module handle
 */
struct wav_writer;


/**
 * @brief WAV file parameters
 */
struct wav_writer_params {
   int fs;                                  ///< Sampling frequency (Hz)
   int bit_depth;                           ///< Sample size (bits)
   int nb_channels;                         ///< Number of channels
   const char *filename;                    ///< Output filename
};


/**
 * @brief Module initialization
 *
 * @param[in] params    : WAV file parameters
 *
 * @note Only PCM supported so far
 *
 * @return Valid module handle if successful, NULL else
 */
struct wav_writer *wav_writer_create(struct wav_writer_params *params);


/**
 * @brief Release module resources
 *
 * @note This method writes the WAV header, and flush & release opened file
 *       descriptor => Don't forget to call it !
 *
 * @param[in] handle    : Module handle
 *
 * @return None
 */
void wav_writer_destroy(struct wav_writer **handle);


/**
 * @brief Write WAV frames to output file
 *
 * @param[in] handle    : Module handle
 * @param[in] data      : Raw data to be written to file
 * @param[in] nb_frames : Number of frames to be written
 *
 * @return nb_frames written if successful, errno (<0) else.
 */
int wav_writer_write(struct wav_writer *handle, void *data, int nb_frames);


#endif /* _WAV_WRITER_H_ */
