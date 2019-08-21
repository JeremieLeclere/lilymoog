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


/*
 * Check detected length name matches one of the supported values
 */
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
 * Event is assumed to respect the following structure:
 *
 *      [NOTE_NAME][RANK_UPDATE][LENGTH_UPDATE]
 *
 *  with
 *
 *  . NOTE_NAME     : Group of 1 or 2 letters, expected to match one of the
 *                    NOTES_NAMES table. This field is mandatory.
 *  . RANK_UPDATE   : Must be a ' or a , character, indicating an octave update.
 *                    This field is optional.
 *  . LENGTH_UPDATE : Must be a number in 
 */
int parse_event(char *token, struct event *event)
{
    int ret = 0;
    int len_pos = 0;
    char *c = token;
    int len_len = 0;
    int name_len = 0;
    int position = 0;
    char len[3] = {0};

    memset(event, 0, sizeof(struct event));

    /* Get note name */
    while ((isalpha(*c)) && (position < strlen(token))) {
        c++;
        name_len++;
        position ++;
    }

    if (name_len > 2) {
        LOGE("Unexpected note name !");
        ret = -EINVAL;
        goto exit;
    }

    event->note[0] = toupper(token[0]);
    if (name_len == 2)
        event->note[1] = tolower(token[1]);

    if (check_note_name(event->note) == -EINVAL) {
        LOGE("Unexpected note name !");
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
            LOGE("Unexpected note length !");
            ret = -EINVAL;
            goto exit;
        }

        len[0] = token[len_pos];
        if (len_len == 2)
            len[1] = token[len_pos + 1];
        event->len_update = atoi(len);

        if (check_len_update(event->len_update) == -EINVAL) {
            LOGE("Unexpected length update !");
            goto exit;
        }
    }

exit:

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
        token = strtok(line, " ");
        while (token) {
            sequence->nb_events++;
            token = strtok(NULL, " ");
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
                     line_index, event_index, token);
            }
            token = strtok(NULL, " ");
            event_index++;
            event_id++;
        }

        line_index++;
    }

exit:

    if (fd)
        fclose(fd);

    return ret;
}
