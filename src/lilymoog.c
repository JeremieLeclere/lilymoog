/***************************************************************************************************
 * @file lilymoog.c
 *
 * @brief Lilypond like syntax based WAV file generator, using a simple Moog like structure (sources)
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
#include <getopt.h>

#include <log.h>
#include <moog.h>
#include <notes.h>
#include <wav_writer.h>
#include <cfg_parser.h>
#include <seq_parser.h>


#define DFT_OUTPUT_FILE     ("output.wav")          ///< Default output file
#define DFT_RANK            (2)
#define DFT_LENGTH          (4)


static void usage(const char *exec_name)
{
    LOGI("%s -c CONFIG -s SCRIPT [-o OUTPUT_FILE] [-p PREFILL] [-P POSTFILL]", exec_name);
    LOGI("");
    LOGI("    Moog sequence generator using provided script and configuration");
    LOGI("");
    LOGI(" -c CONFIG");
    LOGI("    Moog synthesizer configuration and sequence tempo. Please refer to README.md");
    LOGI("    for more details about configurable parameters.");
    LOGI("");
    LOGI(" -s SCRIPT");
    LOGI("    Sequence to be generated, written in lilypond like syntax. Please refer to README.md");
    LOGI("    for more details about syntax.");
    LOGI("");
    LOGI(" -o OUTPUT_FILE");
    LOGI("    Output WAV filename (default: 'output.wav')");
    LOGI("");
    LOGI(" -p PREFILL");
    LOGI("    Prefill generated file with silence.");
    LOGI("    The value must be an positive int value, which will be interpreted as a number of");
    LOGI("    sixteenth notes. The equivalent duration of silence will be inserted at the");
    LOGI("    beginning of generated output file.");
    LOGI("");
    LOGI(" -P POSTFILL");
    LOGI("    Postfill generated file with silence.");
    LOGI("    The value must be an positive int value, which will be interpreted as a number of");
    LOGI("    sixteenth notes. The equivalent duration of silence will be inserted at the");
    LOGI("    end of generated output file.");
    LOGI("");
}


int main(int argc, char *argv[])
{
    float frequency;
    int rank, length;
    struct cfg config;
    float Q, fc, gain;
    struct seq sequence;
    int i, j, k, c, ret;
    int g_ret = EXIT_SUCCESS;
    struct moog *moog = NULL;
    int32_t *output_frame = NULL;
    struct wav_writer *wav = NULL;
    struct wav_writer_params wav_params;

    char *script_file = NULL;
    int nb_prefill_frames = 0;
    int nb_postfill_frames = 0;
    char *configuration_file = NULL;
    char *output_file = DFT_OUTPUT_FILE;

    while ((c = getopt(argc, argv, "hc:s:o:p:P:")) != -1) {
        switch (c) {
        case 'h':
            usage(argv[0]);
            goto exit;
        case 'c':
            configuration_file = optarg;
        break;
        case 's':
            script_file = optarg;
        break;
        case 'o':
            output_file = optarg;
        break;
        case 'p':
            nb_prefill_frames = atoi(optarg);
            if (nb_prefill_frames < 0) {
                LOGE("Unexpected PREFILL value (%d)", nb_prefill_frames);
                usage(argv[0]);
                g_ret = -EINVAL;
                goto exit;
            }
        break;
        case 'P':
        nb_postfill_frames = atoi(optarg);
            if (nb_postfill_frames < 0) {
                LOGE("Unexpected POSTFILL value (%d)", nb_postfill_frames);
                usage(argv[0]);
                g_ret = -EINVAL;
                goto exit;
            }
        break;
        case '?':
            g_ret = EXIT_FAILURE;
            usage(argv[0]);
            goto exit;
        }
    }

    /* Check mandatory arguments */
    if (!configuration_file) {
        LOGE("Missing configuration file");
        usage(argv[0]);
        g_ret = -EINVAL;
        goto exit;
    }

    if (!script_file) {
        LOGE("Missing script file");
        usage(argv[0]);
        g_ret = -EINVAL;
        goto exit;
    }

    /* Parse user configuration */
    ret = parse_cfg(configuration_file, &config);
    if (ret) {
        LOGE("Configuration parsing failure");
        g_ret = EXIT_FAILURE;
        goto exit;
    }

    /* Parse user sequence */
    ret = parse_sequence(script_file, &sequence);
    if (ret) {
        LOGE("Sequence parsing failure");
        g_ret = EXIT_FAILURE;
        goto exit;
    }


    /* Output frame */
    output_frame = (int32_t *)calloc(config.m_params.frame_size, sizeof(int32_t));
    if (!output_frame) {
        LOGE("Output buffer allocation failure");
        g_ret = EXIT_FAILURE;
        goto exit;
    }

    /* Moog init */
    moog = moog_create(&config.m_params);
    if (!moog) {
        LOGE("Failed to initialize Moog module !");
        g_ret = EXIT_FAILURE;
        goto exit;
    }

    /* WAV writer */
    wav_params.fs          = config.m_params.fs;
    wav_params.bit_depth   = 32;
    wav_params.nb_channels = 1;
    wav_params.filename    = output_file;
    wav = wav_writer_create(&wav_params);
    if (!wav) {
        LOGE("Failed to create WAV writer !");
        g_ret = EXIT_FAILURE;
        goto exit;
    }

    /* Set output intensity */
    moog_set_intensity(moog, config.intensity);

    /* Pre-fill with silence */
    moog_toggle(moog, 0);
    for (i = 0; i < nb_prefill_frames; i++) {
        moog_process(moog, output_frame);
        /* QS8.23 to QS.31 */
        for (j = 0; j < config.m_params.frame_size; j++)
            output_frame[j] = output_frame[j] << 8;
        wav_writer_write(wav, output_frame, config.m_params.frame_size);
    }

    /* Main loop */
    rank = DFT_RANK;
    length = DFT_LENGTH;
    for (i = 0; i < sequence.nb_events; i++) {

        /* Silence / note update */
        if (strcmp(sequence.events[i].note, "R") == 0) {
            ret = moog_toggle(moog, 0);
            if (ret)
                LOGE("Failed to toggle Moog OFF");
        } else {
            rank += sequence.events[i].rank_update;
            ret = get_note(rank, sequence.events[i].note, &frequency);
            if (ret) {
                LOGE("Failed to get note frequency !");
                g_ret = EXIT_FAILURE;
                goto exit;
            }
            ret = moog_toggle(moog, 1);
            if (ret) {
                LOGE("Failed to toggle Moog ON !");
                g_ret = EXIT_FAILURE;
                goto exit;
            }
            ret = moog_set_frequency(moog, frequency);
            if (ret) {
                LOGE("Failed to set Moog frequency ! Please consider reducing the attack and/or release time");
                g_ret = EXIT_FAILURE;
                goto exit;
            }
        }

        /* Low pass filter parameters update */
        if ((sequence.events[i].q_update != LP_NO_UPDATE_VALUE)
        ||  (sequence.events[i].fc_update != LP_NO_UPDATE_VALUE)
        ||  (sequence.events[i].gain_update != LP_NO_UPDATE_VALUE)) {

            /* Get current filter parameters */
            ret = moog_filter_get_parameters(moog, &fc, &Q, &gain);
            if (ret) {
                LOGE("Failed to retrieve Moog parameters !");
                g_ret = EXIT_FAILURE;
                goto exit;
            }

            /* Update specified parameters */
            if (sequence.events[i].q_update != LP_NO_UPDATE_VALUE)
                Q = sequence.events[i].q_update;
            if (sequence.events[i].fc_update != LP_NO_UPDATE_VALUE)
                fc = sequence.events[i].fc_update;
            if (sequence.events[i].gain_update != LP_NO_UPDATE_VALUE)
                fc = sequence.events[i].gain_update;

            /* Apply new parameters set */
            ret = moog_filter_set_parameters(moog, fc, Q, gain);
            if (ret) {
                LOGE("Failed to update Moog filter parameters !");
                g_ret = EXIT_FAILURE;
                goto exit;
            }
        }

        /* Length update */
        if (sequence.events[i].len_update != 0)
            length = sequence.events[i].len_update;

        /* Output generation */
        for (j = 0; j < length; j++) {
            moog_process(moog, output_frame);
            /* QS8.23 to QS.31 */
            for (k = 0; k < config.m_params.frame_size; k++)
                output_frame[k] = output_frame[k] << 8;
            wav_writer_write(wav, output_frame, config.m_params.frame_size);
        }

    }

    /* Post-fill with silence */
    moog_toggle(moog, 0);
    for (i = 0; i < nb_postfill_frames; i++) {
        moog_process(moog, output_frame);
        /* QS8.23 to QS.31 */
        for (j = 0; j < config.m_params.frame_size; j++)
            output_frame[j] = output_frame[j] << 8;
        wav_writer_write(wav, output_frame, config.m_params.frame_size);
    }

exit:

    if (sequence.events)
        free(sequence.events);

    if (output_frame)
        free(output_frame);

    moog_destroy(&moog);
    wav_writer_destroy(&wav);

    return g_ret;
}
