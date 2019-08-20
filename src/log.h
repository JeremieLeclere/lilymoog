/***************************************************************************************************
 * @file log.h
 *
 * @brief Log macros
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

#ifndef _LOG_H_
#define _LOG_H_

#include <time.h>
#include <stdio.h>

#define COLOR_RED          "\033[31m"
#define COLOR_GREEN        "\033[32m"
#define COLOR_YELLOW       "\033[33m"
#define COLOR_BLUE         "\033[34m"
#define COLOR_MAGENTA      "\033[35m"
#define COLOR_CYAN         "\033[36m"
#define COLOR_DEFAULT      "\033[39m"

#define ID_INFO            "[I] "
#define ID_NOTIFY          "[N] "
#define ID_DEBUG           "[D] "
#define ID_WARNING         "[W] "
#define ID_ERROR           "[E] "
#define ID_CRITICAL        "[C] "

#define LEVEL_INFO         (1 << 0)
#define LEVEL_DEBUG        (1 << 1)
#define LEVEL_WARNING      (1 << 2)
#define LEVEL_ERROR        (1 << 3)
#define LEVEL_CRITICAL     (1 << 4)
#define LEVEL_NOTIFY       (1 << 5)


#define _LOG(level, id, msg, ...)												\
do																				\
{																				\
   time_t timer;																\
   char buffer[16];																\
   struct tm* tm_info;															\
   time(&timer);																\
   tm_info = localtime(&timer);													\
   strftime(buffer, 14, "[%H:%M:%S] ", tm_info);								\
   printf(level "%s" id msg COLOR_DEFAULT "\n", buffer, ##__VA_ARGS__);			\
} while (0);

#define LOGI(msg, ...)															\
do																				\
{																				\
   _LOG(COLOR_GREEN, ID_INFO, msg, ##__VA_ARGS__)								\
} while (0);

#define LOGE(msg, ...)															\
do																				\
{																				\
   _LOG(COLOR_RED, ID_ERROR, msg, ##__VA_ARGS__)								\
} while (0);

#define LOGW(msg, ...)															\
do																				\
{																				\
   _LOG(COLOR_YELLOW, ID_WARNING, msg, ##__VA_ARGS__)							\
} while (0);

#define LOGD(msg, ...)															\
do																				\
{																				\
   _LOG(COLOR_CYAN, ID_DEBUG, msg, ##__VA_ARGS__)								\
} while (0);


#define LOGC(msg, ...)															\
do																				\
{																				\
   _LOG(COLOR_MAGENTA, ID_CRITICAL, msg, ##__VA_ARGS__)							\
} while (0);

#define LOGN(msg, ...)															\
do																				\
{																				\
   _LOG(COLOR_BLUE, ID_NOTIFY, msg, ##__VA_ARGS__)								\
} while (0);


#endif
