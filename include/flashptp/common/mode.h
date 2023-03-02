/*
 * @file mode.h
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
 *
 * The abstract class Mode provides a pattern for the classes client::ClientMode
 * and server::ServerMode. A Mode object pointer needs to be passed to the function
 * network::recv, so that the specific implementation of the pure virtual method
 * onMsgReceived can be called and received packets can be processed.
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

#ifndef INCLUDE_FLASHPTP_COMMON_MODE_H_
#define INCLUDE_FLASHPTP_COMMON_MODE_H_

#include <flashptp/common/thread.h>

namespace flashptp {

class FlashPTP;

class Mode : public Thread {
public:
    inline Mode(FlashPTP *flashPTP) { _flashPTP = flashPTP; }

    /*
     * Pure virtual method to be implemented by deriving classes (ClientMode, ServerMode).
     * The method is being called by network::recv on receipt of an incoming PTP Message.
     */
    virtual void onMsgReceived(PTP2Message *msg, int len, const struct sockaddr_storage *srcSockaddr,
            const struct sockaddr_storage *dstSockaddr, PTPTimestampLevel timestampLevel,
            const struct timespec *timestamp) = 0;

protected:
    bool alwaysEnabled() const { return false; }

    FlashPTP *_flashPTP;
};

}

#endif /* INCLUDE_FLASHPTP_COMMON_MODE_H_ */
