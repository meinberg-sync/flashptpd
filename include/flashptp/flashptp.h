/*
 * @file flashptp.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class FlashPTP provides an interface to the flashPTP library. It takes
 * a JSON-formatted configuration and runs flashPTP client and/or server mode.
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

#ifndef INCLUDE_FLASHPTP_FLASHPTP_H_
#define INCLUDE_FLASHPTP_FLASHPTP_H_

#include <flashptp/network/network.h>
#include <flashptp/client/clientMode.h>
#include <flashptp/server/serverMode.h>

#define FLASH_PTP_JSON_CFG_LOGGING                      "logging"

namespace flashptp {

class FlashPTP {
public:
    inline ~FlashPTP() { if (_running) stop(); network::exit(); cppLog::exit(); }

    static bool validateConfig(const Json &config, std::vector<std::string> *errs);
    bool setConfig(const Json &config, std::vector<std::string> *errs = nullptr);

    inline client::ClientMode &clientMode() { return _clientMode; }
    inline server::ServerMode &serverMode() { return _serverMode; }

    // Enable forwarding of parts of Sync Response sequences to client mode (if enabled and running)
    inline void onRespReceived(PTP2Message *msg, int len, const struct sockaddr_storage *srcSockaddr,
            const struct sockaddr_storage *dstSockaddr, PTPTimestampLevel timestampLevel,
            const struct timespec *timestamp)
    { _clientMode.onMsgReceived(msg, len, srcSockaddr, dstSockaddr, timestampLevel, timestamp); }

    // Enable forwarding of parts of Sync Request sequences to server mode (if enabled and running)
    inline void onReqReceived(PTP2Message *msg, int len, const struct sockaddr_storage *srcSockaddr,
            const struct sockaddr_storage *dstSockaddr, PTPTimestampLevel timestampLevel,
            const struct timespec *timestamp)
    { _serverMode.onMsgReceived(msg, len, srcSockaddr, dstSockaddr, timestampLevel, timestamp); }

    // Start client and/or server mode worker threads.
    void start();
    // Stop client and/or server mode worker threads.
    void stop();

private:
    bool _running{ false };

    client::ClientMode _clientMode{ this };
    server::ServerMode _serverMode{ this };
};

}

#endif /* INCLUDE_FLASHPTP_FLASHPTP_H_ */
