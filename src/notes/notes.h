/***************************************************************************************************
 * @file notes.h
 *
 * @brief Notes management module (header)
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

#ifndef _NOTES_H_
#define _NOTES_H_


#include <errno.h>


#define MIN_RANK    (0)
#define MAX_RANK    (8)


/**
 * @brief Check if provided note name is correct
 *
 *  The supported notes names are:
 * 
 *      - "A", "B", "C", "D", "E", "F", "G", each of them possibly suffixed
 *        with a "b" ('bémol') or a "d" ('dièse'),
 *      - "R", which stands for a rest.
 *
 * @param[in] note_name : Well ... Note name
 *
 * @return 0 if successful, 0 > errno else
 */
int check_note_name(const char *note_name);


/**
 * @brief Get frequency associated to specified note
 *
 * @param[in]  rank         : Note octave rank (like 4, in A4 = 440Hz)
 * @param[in]  note         : Note name
 * @param[out] frequency    : Note frequency (in Hz)
 *
 * @return 0 if successful, 0 > errno else
 */
int get_note(int rank, const char *note, float *frequency);


#endif /* _NOTES_H_ */
