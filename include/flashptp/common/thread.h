/*
 * @file thread.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The abstract class Thread provides a pattern for all classes within flashPTP,
 * that need to run any kind of worker thread. All that needs to be done is to
 * provide a derived class that implements the pure virtual methods alwaysEnabled,
 * threadName and threadFunc.
 *
 * At the moment, this class is derived by the classes Mode (client::ClientMode,
 * server::ServerMode), client::Server, server::Listener and network::Inventory.
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

#ifndef INCLUDE_FLASHPTP_COMMON_THREAD_H_
#define INCLUDE_FLASHPTP_COMMON_THREAD_H_

#include <flashptp/common/defines.h>

namespace flashptp {

class Thread {
public:
    // Try to start the worker thread (threadFunc).
    bool start();
    // Stop the worker thread (threadFunc), if running.
    void stop();

protected:
    virtual inline ~Thread()
    {
        if (_running)
            stop();
    }

    /*
     * This method can be used to overrule the _enabled member, which needs to be set via configuration.
     * If you want your worker thread to be always enabled, return true, otherwise return false.
     */
    virtual bool alwaysEnabled() const = 0;
    // This method shall return a representative name of the worker thread for logging purpose.
    virtual const char *threadName() const = 0;
    // This method contains the actual work that shall be executed in the worker thread.
    virtual void threadFunc() = 0;

    std::atomic_bool _enabled{ false };
    std::atomic_bool _running{ false };

private:
    void run();

    std::thread* _thread{ nullptr };
};

}

#endif /* INCLUDE_FLASHPTP_COMMON_THREAD_H_ */
