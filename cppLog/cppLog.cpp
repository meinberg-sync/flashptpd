/*
 * @file cppLog.cpp
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
 *
 * =============================================================================
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * =============================================================================
 *
 */

#include "cppLog.h"

namespace cppLog {

static Log *__log;

void init(const Json &config)
{
    exit();
    __log = new Log(config);
}

void errorf(const char *fmt, ...)
{
    if (!__log || !__log->hasSeverity(LogSeverity::error))
        return;

    va_list args;
    va_start(args, fmt);
    __log->logf(LogSeverity::error, fmt, args);
    va_end(args);
}

void warningf(const char *fmt, ...)
{
    if (!__log || !__log->hasSeverity(LogSeverity::warning))
        return;

    va_list args;
    va_start(args, fmt);
    __log->logf(LogSeverity::warning, fmt, args);
    va_end(args);
}

void infof(const char *fmt, ...)
{
    if (!__log || !__log->hasSeverity(LogSeverity::info))
        return;

    va_list args;
    va_start(args, fmt);
    __log->logf(LogSeverity::info, fmt, args);
    va_end(args);
}

void debugf(const char *fmt, ...)
{
    if (!__log || !__log->hasSeverity(LogSeverity::debug))
        return;

    va_list args;
    va_start(args, fmt);
    __log->logf(LogSeverity::debug, fmt, args);
    va_end(args);
}

void tracef(const char *fmt, ...)
{
    if (!__log || !__log->hasSeverity(LogSeverity::trace))
        return;

    va_list args;
    va_start(args, fmt);
    __log->logf(LogSeverity::trace, fmt, args);
    va_end(args);
}

void elevenf(const char *fmt, ...)
{
    if (!__log || !__log->hasSeverity(LogSeverity::eleven))
        return;

    va_list args;
    va_start(args, fmt);
    __log->logf(LogSeverity::eleven, fmt, args);
    va_end(args);
}

void exit()
{
    if (__log)
        delete __log;
    __log = nullptr;
}

}
