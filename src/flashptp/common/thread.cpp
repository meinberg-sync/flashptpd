/*
 * @file thread.cpp
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

#include <flashptp/common/thread.h>

namespace flashptp {

bool Thread::start()
{
    bool success;

    if (_running)
        stop();

    if (!alwaysEnabled() && !_enabled)
        return false;

    try {
        cppLog::debugf("Starting %s", threadName());
        _running = true;
        _thread = new std::thread(&Thread::run, this);
    }
    catch (const std::system_error& e) {
        delete _thread;
        _thread = nullptr;
        _running = false;
        cppLog::errorf("%s could not be started: %s", threadName(), e.what());
    }

    if (_running)
        cppLog::infof("%s started, successfully", threadName());

out:
    return _running;
}

void Thread::stop()
{
    if (!_running)
        return;

    _running = false;
    if (!_thread)
        goto out;

    cppLog::debugf("Stopping %s, waiting for thread to stop execution...", threadName());

    if (_thread->joinable())
        _thread->join();

    delete _thread;
    _thread = nullptr;

out:
    cppLog::infof("%s stopped", threadName());
}

void Thread::run()
{
    pid_t tid = gettid();
    cppLog::tracef("%s thread (%d) started", threadName(), tid);

    threadFunc();
    _running = false;

    cppLog::tracef("%s thread (%d) stopped", threadName(), tid);
}

}
