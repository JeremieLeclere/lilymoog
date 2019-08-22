/***************************************************************************************************
 * @file seq_parser.c
 *
 * @brief Sequence parsing module (sources)
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <log.h>
#include <notes.h>
#include <seq_parser.h>

#define NB_LENGTHS      (5)

static int NOTES_LENGTH[NB_LENGTHS] = {1, 2, 4, 8, 16};


/* Convert user duration to corresponding number of sixteenth notes */
static int len_2_nb_sixteenth(int len)
{
    int ret;

    switch (len) {
    case 1:
        ret = 16;
    break;
    case 2:
        ret = 8;
    break;
    case 4:
        ret = 4;
    break;
    case 8:
        ret = 2;
    break;
    case 16:
        ret = 1;
    break;
    default:
        ret = -1;
    }

    return ret;
}


/* Check detected length name matches one of the supported values */
static int check_len_update(int len_update)
{
    int i, ret = -EINVAL;

    for (i = 0; i < NB_LENGTHS; i++) {
        if (len_update == NOTES_LENGTH[i]) {
            ret = 0;
            goto exit;
        }
    }

exit:

    return ret;
}

/*
 * Parse a MOOG_UPDATE command, which is assumed to respect following syntax:
 *
 *      KEY:VALUE
 *
 *  . KEY must be in ['q', 'fc', 'gain']
 *  . VALUE is a float value representing new KEY value
 */
static int parse_moog_event(char *token, struct event *event)
{
    int ret = 0;
    int position = 0;
    int field_len = 0;
    char field[5] = {0};
    float value;

    while ((token[position] != ':') && (position++ < strlen(token)))
        field_len++;

    if (field_len > 4) {
        LOGE("%s: Unsupported moog update type: %s", __func__, token);
        ret = -EINVAL;
        goto exit;
    }

    strncpy(field, token, field_len);
    value = atof(token + field_len + 1);

    if (strcmp(field, "q") == 0) {
        event->q_update = value;
    } else if (strcmp(field, "fc") == 0) {
        event->fc_update = value;
    } else if (strcmp(field, "gain") == 0) {
        event->gain_update = value;
    } else {
        LOGE("%s: Unsupported moog update type: %s", __func__, token);
        ret = -EINVAL;
        goto exit;
    }

exit:

    return ret;
}

/*
 * Parse the MOOG_UPDATE field, assumed to be a series of configuration elements separated
 * by a space.
 */
static int parse_moog_update(char *data, struct event *event)
{
    int ret = 0;
    char *ctx = NULL;
    char *token = NULL;

    token = strtok_r(data, ",", &ctx);
    while (token) {
        ret = parse_moog_event(token, event);
        if (ret)
            goto exit;
        token = strtok_r(NULL, ",", &ctx);
    }

exit:

    return ret;
}


/*
 * Event is assumed to respect the following structure:
 *
 *      [NOTE_NAME][RANK_UPDATE][LENGTH_UPDATE][MOOG_UPDATE]
 *
 *  with
 *
 *  . NOTE_NAME     : Group of 1 or 2 letters, expected to match one of the
 *                    NOTES_NAMES table. This field is mandatory.
 *  . RANK_UPDATE   : Must be a ' or a , character, indicating an octave update.
 *                    This field is optional.
 *  . LENGTH_UPDATE : Must be a number in 1, 2, 4, 8, 16
 *  . MOOG_UPDATE   : Moog parameters update, parsed with parse_moog_update
 */
static int parse_event(char *token, struct event *event)
{
    int ret = 0;
    int len_pos = 0;
    char *c = token;
    int len_len = 0;
    int name_len = 0;
    int position = 0;
    char len[3] = {0};
    char *sub_section = NULL;
    int sub_section_length = 0;
    int sub_section_position = 0;

    /* Initialize event */
    memset(event, 0, sizeof(struct event));
    event->q_update    = LP_NO_UPDATE_VALUE;
    event->fc_update   = LP_NO_UPDATE_VALUE;
    event->gain_update = LP_NO_UPDATE_VALUE;

    /* Get note name */
    while ((isalpha(*c)) && (position < strlen(token))) {
        c++;
        name_len++;
        position ++;
    }

    if (name_len > 2) {
        LOGE("%s: Unexpected note name !", __func__);
        ret = -EINVAL;
        goto exit;
    }

    event->note[0] = toupper(token[0]);
    if (name_len == 2)
        event->note[1] = tolower(token[1]);

    if (check_note_name(event->note) == -EINVAL) {
        LOGE("%s: Unexpected note name !", __func__);
        goto exit;
    }

    /* Check for a rank update */
    while (((*c == '\'') || (*c == ',')) && (position < strlen(token))) {
        if (*c == '\'')
            event->rank_update++;
        else
            event->rank_update--;
        c++;
        position++;
    }

    /* Check for a length update */
    len_pos = position;
    while ((isdigit(*c)) && (position < strlen(token))) {
        c++;
        len_len++;
        position ++;
    }

    if (len_len == 0) {

        /* Field already set to 0 */

    } else {
        if (len_len > 2) {
            LOGE("%s: Unexpected note length !", __func__);
            ret = -EINVAL;
            goto exit;
        }

        len[0] = token[len_pos];
        if (len_len == 2)
            len[1] = token[len_pos + 1];
        event->len_update = atoi(len);

        if (check_len_update(event->len_update) == -EINVAL) {
            LOGE("%s: Unexpected length update !", __func__);
            goto exit;
        }

        event->len_update = len_2_nb_sixteenth(event->len_update);
    }

    if (position == strlen(token))
        goto exit;

    /* Search for eventual additional section */
    if (token[position] == '[') {
        sub_section_position = ++position;
        while ((token[position] != ']') && (position < strlen(token))) {
            position++;
            sub_section_length++;
        }
        if (token[position] != ']') {
            LOGE("%s: Unterminated [] section !", __func__);
            ret = -EINVAL;
            goto exit;
        }

        if (sub_section_length == 0) {
            LOGW("%s: void sub section detected '%s'", __func__, token);
            goto exit;
        }

        sub_section = (char *)calloc(sub_section_length + 1, sizeof(char));
        if (!sub_section) {
            LOGE("%s: Failed to allocate sub_section string !", __func__);
            ret = -ENOMEM;
            goto exit;
        }
        strncpy(sub_section, token + sub_section_position, sub_section_length);
        ret = parse_moog_update(sub_section, event);
    }

exit:

    if (sub_section)
        free(sub_section);

    return ret;
}


int parse_sequence(const char *filename, struct seq *sequence)
{
    char *c;
    int ret = 0;
    int event_id;
    int line_index;
    int event_index;
    FILE *fd = NULL;
    size_t line_size;
    char *ctx = NULL;
    char *line = NULL;
    char *token = NULL;

    if ((!filename) || (!sequence)) {
        ret = -EINVAL;
        goto exit;
    }

    fd = fopen(filename, "r");
    if (!fd) {
        LOGE("%s: Failed to open file '%s'", __func__, filename);
        ret = -EINVAL;
        goto exit;
    }

    /* Get number of events */
    sequence->nb_events = 0;
    while (getline(&line, &line_size, fd) != -1) {

        /* Split line in space separated elements */
        token = strtok_r(line, " ", &ctx);
        while (token) {
            sequence->nb_events++;
            token = strtok_r(NULL, " ", &ctx);
        }
    }
    sequence->events = (struct event *)calloc(sequence->nb_events,
                                              sizeof(struct event));
    if (!sequence->events) {
        LOGE("%s: Failed to allocate events table !", __func__);
        ret = -ENOMEM;
        goto exit;
    }

    /* Parse events */
    event_id = 0;
    line_index = 0;
    fseek(fd, 0, SEEK_SET);
    while (getline(&line, &line_size, fd) != -1) {

        event_index = 0;

        /* Remove \n character if any */
        if ((c = strchr(line, '\n')) != NULL)
            *c = '\0';

        /* Split line in space separated elements */
        token = strtok(line, " ");
        while (token) {
            /* Try to interpret user command */
            ret = parse_event(token, &sequence->events[event_id]);
            if (ret) {
                LOGE("%s: Line %d, event %d: '%s'", __func__,
                     line_index + 1, event_index + 1, token);
            }
            token = strtok(NULL, " ");
            event_index++;
            event_id++;
        }

        line_index++;
    }

exit:

    if (line)
        free(line);

    if (fd)
        fclose(fd);

    return ret;
}
