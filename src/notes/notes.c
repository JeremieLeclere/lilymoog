/***************************************************************************************************
 * @file notes.c
 *
 * @brief Notes management module (sources)
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

#include <math.h>
#include <string.h>

#include <notes.h>


#define C0  (16.351597831287414)
#define C1  (32.703195662574830)
#define C2  (65.406391325149660)
#define C3  (130.81278265029930)
#define C4  (261.62556530059800)
#define C5  (523.25113060119720)
#define C6  (1046.5022612023945)
#define C7  (2093.0045224047890)
#define C8  (4186.0090448095780)


#define NB_NOTES        (22)
static char *NOTES_NAMES[NB_NOTES] = {
                                        "A", "Ab", "Ad", "B", "Bb", "Bd",
                                        "C", "Cb", "Cd", "D", "Db", "Dd",
                                        "E", "Eb", "Ed", "F", "Fb", "Fd",
                                        "G", "Gb", "Gd", "R"
                                     };


/* Check note name matches one of the supported notes */
int check_note_name(const char *note_name)
{
    int i, ret = -EINVAL;

    for (i = 0; i < NB_NOTES; i++) {
        if (strcmp(note_name, NOTES_NAMES[i]) == 0) {
            ret = 0;
            goto exit;
        }
    }

exit:

    return ret;
}


int get_note(int rank, const char *note, float *frequency)
{
    int ret = 0;
    float base, scale;

    *frequency = -1;

    switch (rank) {
    case 0:
        base = C0;
        break;
    case 1:
        base = C1;
        break;
    case 2:
        base = C2;
        break;
    case 3:
        base = C3;
        break;
    case 4:
        base = C4;
        break;
    case 5:
        base = C5;
        break;
    case 6:
        base = C6;
        break;
    case 7:
        base = C7;
        break;
    case 8:
        base = C8;
        break;
    default:
        ret = -EINVAL;
        goto exit;
    }

    if ((strcmp(note, "C") == 0)
    ||  (strcmp(note, "Bs") == 0)) {
        scale = 1.0;
    } else if ((strcmp(note, "Cs") == 0)
    ||         (strcmp(note, "Dd") == 0)) {
        scale = pow(2, 1.0/12);
    } else if (strcmp(note, "D") == 0) {
        scale = pow(2, 2.0/12);
    } else if ((strcmp(note, "Ds") == 0)
    ||         (strcmp(note, "Ed") == 0)) {
        scale = pow(2, 3.0/12);
    } else if ((strcmp(note, "E") == 0)
    ||         (strcmp(note, "Fd") == 0)) {
        scale = pow(2, 4.0/12);
    } else if ((strcmp(note, "F") == 0)
    ||         (strcmp(note, "Es") == 0)) {
        scale = pow(2, 5.0/12);
    } else if ((strcmp(note, "Fs") == 0)
    ||         (strcmp(note, "Gd") == 0)) {
        scale = pow(2, 6.0/12);
    } else if (strcmp(note, "G") == 0) {
        scale = pow(2, 7.0/12);
    } else if ((strcmp(note, "Gs") == 0)
    ||       (strcmp(note, "Ad") == 0)) {
        scale = pow(2, 8.0/12);
    } else if (strcmp(note, "A") == 0) {
        scale = pow(2, 9.0/12);
    } else if ((strcmp(note, "As") == 0)
    ||       (strcmp(note, "Bd") == 0)) {
        scale = pow(2, 10.0/12);
    } else if ((strcmp(note, "B") == 0)
    ||         (strcmp(note, "Cd") == 0)) {
        scale = pow(2, 11.0/12);
    } else {
        ret = -EINVAL;
        goto exit;
    }

    *frequency = base * scale;

exit:

    return ret;
}
