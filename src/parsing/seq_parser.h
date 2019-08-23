/***************************************************************************************************
 * @file seq_parser.h
 *
 * @brief Sequence parsing module (header)
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

#ifndef _SEQ_PARSER_H_
#define _SEQ_PARSER_H_


#include <errno.h>


/**
 * @brief Arbitrary value (not supposed to be a realistic request) used to initialize low pass
 * update parameter in event structure, as zero might be a a realisatic request.
 * (have to find something smarter than that ...)
 */
#define LP_NO_UPDATE_VALUE     (-987341.5)


/**
 * @brief Note event structure
 */
struct event {
    char note[3];                           ///< Note name
    int len_update;                         ///< Note length update value (number of sixteenth notes)
    int rank_update;                        ///< Note rank update value
    float q_update;                         ///< Low pass Q factor update
    float fc_update;                        ///< Low pass cutoff frequency update
    float gain_update;                      ///< Low pass gain update
    float fc_sweep;                         ///< Low pass cutoff frequency start
};


/**
 * @brief Sequence structure
 */
struct seq {
    int nb_events;                          ///< Number of events in the sequence
    struct event *events;                   ///< List of events
};


/**
 * @brief Parse provided sequence file
 *
 * @param[in]  filename : User sequence filename
 * @param[out] sequence : Parsed sequence structure
 *
 * @return 0 if successful, 0 > errno else
 */
int parse_sequence(const char *filename, struct seq *sequence);


#endif /* _SEQ_PARSER_H_ */
