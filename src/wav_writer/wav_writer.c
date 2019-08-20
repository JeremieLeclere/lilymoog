/***************************************************************************************************
 * @file wav_writer.c
 *
 * @brief WAV file writer module (sources)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wav_writer.h>


#define DATA_OFFSET        (44)
#define HEADER_DATA        ("data")
#define HEADER_FMT         ("fmt ")
#define HEADER_RIFF        ("RIFF")
#define HEADER_WAVE        ("WAVE")


struct wav_writer {
   int  fs;                                                 ///< Sampling frequency (Hz)
   FILE *fd;                                                ///< Output file descriptor
   int  bit_depth;                                          ///< Sample size (bits)
   int  frame_size;                                         ///< Frame size (bytes)
   int  nb_channels;                                        ///< Number of channels per frame
   int  nb_frames_written;                                  ///< Number of frames written to file
};


/* Helper: Write 4 characters length string to file */
static void _write_char_value(FILE *fd, const char *value)
{
   char tmp[5];
   strncpy(tmp, value, 4);
   fwrite(&tmp, sizeof(char), 4, fd);
}


/* Helper: Write 2 bytes value to file */
static void _write_16bits_value(FILE *fd, int16_t value)
{
   fwrite(&value, sizeof(int16_t), 1, fd);
}


/* Helper: Write 2 bytes value to file */
static void _write_32bits_value(FILE *fd, int32_t value)
{
   fwrite(&value, sizeof(int32_t), 1, fd);
}


/*
 * Header syntax based on following description:
 *
 *    http://soundfile.sapp.org/doc/WaveFormat/
 */
static void _write_header(struct wav_writer *handle)
{
   uint32_t subchunk2size;

   subchunk2size = handle->nb_frames_written * handle->frame_size;

   fseek(handle->fd, 0, SEEK_SET);

   /* ChunkID */
   _write_char_value(handle->fd, HEADER_RIFF);

   /* ChunkSize */
   _write_32bits_value(handle->fd, 36 + subchunk2size);

   /* Format */
   _write_char_value(handle->fd, HEADER_WAVE);

   /* Subchunk1ID */
   _write_char_value(handle->fd, HEADER_FMT);

   /* Subchunk1Size */
   _write_32bits_value(handle->fd, 16);

   /* AudioFormat */
   _write_16bits_value(handle->fd, 1);

   /* NumChannels */
   _write_16bits_value(handle->fd, handle->nb_channels);

   /* SampleRate */
   _write_32bits_value(handle->fd, handle->fs);

   /* ByteRate */
   _write_32bits_value(handle->fd, handle->fs * handle->frame_size);

   /* BlockAlign */
   _write_16bits_value(handle->fd, handle->frame_size);

   /* BitsPerSample */
   _write_16bits_value(handle->fd, handle->bit_depth);

   /* Subchunk2ID */
   fseek(handle->fd, 36, SEEK_SET);
   _write_char_value(handle->fd, HEADER_DATA);

   /* Subchunk2Size */
   _write_32bits_value(handle->fd, subchunk2size);
}


struct wav_writer *wav_writer_create(struct wav_writer_params *params)
{
   struct wav_writer *handle = NULL;

    if (!params)
        goto failure;

   handle = calloc(1, sizeof(struct wav_writer));
   if (!handle)
      goto failure;

   handle->fd = fopen(params->filename, "wb");
   if (!(handle->fd))
      goto failure;

   handle->fs          = params->fs;
   handle->bit_depth   = params->bit_depth;
   handle->nb_channels = params->nb_channels;
   handle->frame_size  = handle->nb_channels * (handle->bit_depth >> 3);

   /* Set current position to data section */
   fseek(handle->fd, DATA_OFFSET, SEEK_SET);

   return handle;

failure:

   wav_writer_destroy(&handle);

   return NULL;
}


void wav_writer_destroy(struct wav_writer **handle)
{
    if ((!handle) || (!(*handle)))
        goto exit;

    if ((*handle)->fd) {
        _write_header(*handle);
        fclose((*handle)->fd);
    }

    free(*handle);
    *handle = NULL;

exit:

    return;
}


int wav_writer_write(struct wav_writer *handle, void *data, int nb_frames)
{
    int ret;

    if ((!handle) || (!data)) {
        ret = -EINVAL;
        goto exit;
    }

    ret = fwrite(data, handle->frame_size, nb_frames, handle->fd);
    handle->nb_frames_written += ret;

exit:

    return ret;
}
