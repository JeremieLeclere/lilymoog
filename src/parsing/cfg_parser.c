/***************************************************************************************************
 * @file cfg_parser.c
 *
 * @brief Configuration parsing module (sources)
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
#include <string.h>

#include <log.h>
#include <cfg_parser.h>


/*
 * Configuration default values
 *
 *  Those are the values applied to the returned configuration if not
 * overriden in provided user configuration.
 */
#define DFT_BPM             (94)
#define SIXTEENTH           (0.25)
#define DFT_FRAME_SIZE      ((int)(60 * DFT_FS * SIXTEENTH / DFT_BPM))
#define DFT_FS              (48000)
#define DFT_LP_Q            (1.5)
#define DFT_LP_FC           (400.0)
#define DFT_LP_GAIN         (1.0)
#define DFT_ATTACK_TIME     (25.0)
#define DFT_DECAY_TIME      (15.0)
#define DFT_SUSTAIN_FACTOR  (0.7)
#define DFT_RELEASE_TIME    (10.0)
#define DFT_OSC_MODE        (WAVE_MODE_SAW)
#define DFT_OSC_COUPLING    (MOOG_OSC_COUPLING_FIFTH)
#define DFT_INTENSITY       (0.6)


#define NB_FIELDS   (12)
static char *config_fields[NB_FIELDS] = {
    "tempo",            ///< Sequence tempo (bpm, int in [1..])
    "fs",               ///< Sampling frequency (Hz, float in [1..)
    "lp_fc",            ///< Moog low pass cutoff frequency (Hz, float in [1..fs/2[)
    "lp_Q",             ///< Moog low pass Q factor (float in ]0..])
    "lp_gain",          ///< Moog low pass gain (dB, float)
    "attack_time",      ///< Moog ADSR attack time (ms, int in [1..])
    "decay_time",       ///< Moog ADSR decay time (ms, int in [1..])
    "sustain",          ///< Moog ADSR sustain factor (float in ]0..1])
    "release_time",     ///< Moog ADSR release time (ms, int in [1..])
    "waveform",         ///< Moog generators waveform (const char in ['saw', 'sine', 'square'])
    "coupling",         ///< Moog generators coupling (const char in ['none', 'third_minor', 'third_major', 'fifth', 'otcave'])
    "intensity"         ///< Moog output intensity (float in ]0, 1])
};


/* Check key matches one of the supported configurable fields */
static int check_key(const char *key)
{
    int i, ret = -EINVAL;

    for (i = 0; i < NB_FIELDS; i++) {
        if (strcmp(key, config_fields[i]) == 0) {
            ret = 0;
            goto exit;
        }
    }

exit:

    return ret;
}


/* Check provided {key,value}, and fill configuration structure */
static int fill_config_element(const char *key, const char *value, struct cfg *configuration)
{
    int ivalue;
    int ret = 0;
    float fvalue;

    if (strcmp(key, "tempo") == 0) {
        fvalue = atof(value);
        if (fvalue <= 0) {
            LOGE("%s: %s must be > 0 (%f provided)", __func__, key, fvalue);
            ret = -EINVAL;
            goto exit;
        }
        configuration->tempo = fvalue;
    } else if (strcmp(key, "fs") == 0) {
        fvalue = atof(value);
        if (fvalue <= 0) {
            LOGE("%s: %s must be > 0 (%f provided)", __func__, key, fvalue);
            ret = -EINVAL;
            goto exit;
        }
        configuration->m_params.fs = fvalue;
    } else if (strcmp(key, "lp_fc") == 0) {
        fvalue = atof(value);
        if ((fvalue <= 0) || (fvalue >= (configuration->m_params.fs)/2)){
            LOGE("%s: %s must be in ]0, fs/2[ (%f provided)", __func__, key, fvalue);
            ret = -EINVAL;
            goto exit;
        }
        configuration->m_params.fc = fvalue;
    } else if (strcmp(key, "lp_Q") == 0) {
        fvalue = atof(value);
        if (fvalue <= 0) {
            LOGE("%s: %s must be > 0 (%f provided)", __func__, key, fvalue);
            ret = -EINVAL;
            goto exit;
        }
        configuration->m_params.Q = fvalue;
    } else if (strcmp(key, "lp_gain") == 0) {
        configuration->m_params.gain = atof(value);
    } else if (strcmp(key, "attack_time") == 0) {
        ivalue = atoi(value);
        if (ivalue <= 0) {
            LOGE("%s: %s must be > 0 (%d provided)", __func__, key, ivalue);
            ret = -EINVAL;
            goto exit;
        }
        configuration->m_params.attack_time = ivalue;
    } else if (strcmp(key, "decay_time") == 0) {
        ivalue = atoi(value);
        if (ivalue <= 0) {
            LOGE("%s: %s must be > 0 (%d provided)", __func__, key, ivalue);
            ret = -EINVAL;
            goto exit;
        }
        configuration->m_params.decay_time = ivalue;
    } else if (strcmp(key, "sustain") == 0) {
        fvalue = atof(value);
        if ((fvalue <= 0) || (fvalue >= 1)){
            LOGE("%s: %s must be in ]0, 1[ (%f provided)", __func__, key, fvalue);
            ret = -EINVAL;
            goto exit;
        }
        configuration->m_params.sustain = fvalue;
    } else if (strcmp(key, "release_time") == 0) {
        ivalue = atoi(value);
        if (ivalue <= 0) {
            LOGE("%s: %s must be > 0 (%d provided)", __func__, key, ivalue);
            ret = -EINVAL;
            goto exit;
        }
        configuration->m_params.release_time = ivalue;
    } else if (strcmp(key, "waveform") == 0) {
        if (strcmp(value, "saw") == 0) {
            configuration->m_params.osc_mode = WAVE_MODE_SAW;
        } else if (strcmp(value, "sine") == 0) {
            configuration->m_params.osc_mode = WAVE_MODE_SINE;
        } else if (strcmp(value, "square") == 0) {
            configuration->m_params.osc_mode = WAVE_MODE_SQUARE;
        } else {
            LOGE("%s: %s must be in [\"saw\", \"sine\", \"square\"] (%s provided)",
                 __func__, key, value);
        }
    } else if (strcmp(key, "coupling") == 0) {
        if (strcmp(value, "none") == 0) {
            configuration->m_params.coupling = MOOG_OSC_COUPLING_NONE;
        } else if (strcmp(value, "third_minor") == 0) {
            configuration->m_params.coupling = MOOG_OSC_COUPLING_THIRD_MINOR;
        } else if (strcmp(value, "third_major") == 0) {
            configuration->m_params.coupling = MOOG_OSC_COUPLING_THIRD_MAJOR;
        } else if (strcmp(value, "fifth") == 0) {
            configuration->m_params.coupling = MOOG_OSC_COUPLING_FIFTH;
        } else if (strcmp(value, "octave") == 0) {
            configuration->m_params.coupling = MOOG_OSC_COUPLING_OCTAVE;
        } else {
            LOGE("%s: %s must be in [\"none\", \"third_minor\", \"third_major\", \
                 \"fifth\", \"octave\"] (%s provided)", __func__, key, value);
        }
    } else if (strcmp(key, "intensity") == 0) {
        fvalue = atof(value);
        if ((fvalue <= 0) || (fvalue > 1)){
            LOGE("%s: %s must be in ]0, 1] (%f provided)", __func__, key, fvalue);
            ret = -EINVAL;
            goto exit;
        }
        configuration->intensity = fvalue;
    }

exit:

    return ret;
}


int get_default_config(struct cfg *configuration)
{
    int ret = 0;

    if (!configuration) {
        ret = -EINVAL;
        goto exit;
    }

    configuration->tempo                    = DFT_BPM;
    configuration->m_params.fs              = DFT_FS;
    configuration->m_params.frame_size      = DFT_FRAME_SIZE;
    configuration->m_params.fc              = DFT_LP_FC;
    configuration->m_params.Q               = DFT_LP_Q;
    configuration->m_params.gain            = DFT_LP_GAIN;
    configuration->m_params.attack_time     = DFT_ATTACK_TIME;
    configuration->m_params.decay_time      = DFT_DECAY_TIME;
    configuration->m_params.sustain         = DFT_SUSTAIN_FACTOR;
    configuration->m_params.release_time    = DFT_RELEASE_TIME;
    configuration->m_params.osc_mode        = DFT_OSC_MODE;
    configuration->m_params.coupling        = DFT_OSC_COUPLING;

exit:

    return ret;
}


int parse_cfg(const char *filename, struct cfg *configuration)
{
    char *c;
    int ret = 0;
    char key[64];
    char value[64];
    FILE *fd = NULL;
    size_t line_size;
    char *line = NULL;
    char *token = NULL;

    if ((!filename) || (!configuration)) {
        ret = -EINVAL;
        goto exit;
    }

    fd = fopen(filename, "r");
    if (!fd) {
        LOGE("%s: Failed to open file '%s'", __func__, filename);
        ret = -EINVAL;
        goto exit;
    }

    /* Load default values, in case not overriden in provided file */
    get_default_config(configuration);

    while (getline(&line, &line_size, fd) != -1) {

        /* Remove \n character if any */
        if ((c = strchr(line, '\n')) != NULL)
            *c = '\0';

        token = strtok(line, "=");
        if (!token)
            continue;
        strncpy(key, token, 64);

        if (check_key(key) == -EINVAL) {
            LOGE("%s: Unsupported config field ('%s')", __func__, key);
            ret = -EINVAL;
            goto exit;
        }

        token = strtok(NULL, "=");
        if (!token)
            continue;
        strncpy(value, token, 64);

        if (fill_config_element(key, value, configuration)) {
            LOGE("Configuration parsing error");
            ret = -EINVAL;
            goto exit;
        }
    };

    configuration->m_params.frame_size = (int)(60 * configuration->m_params.fs
                                               * SIXTEENTH / configuration->tempo);

exit:

    if (line)
        free(line);

    if (fd)
        fclose(fd);

    return ret;
}
