/*
 * @file serverMode.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class ServerMode implements the abstract class Mode and represents the
 * server mode within flashPTP. Its worker thread starts all configured listeners
 * (their worker threads) and periodically checks stored requests (processed by
 * the listeners) for timeouts.
 *
 * It provides the method onMsgReceived, which is to be called by the listeners
 * whenever a part of a Sync Request sequence (Sync or Follow Up Message) has
 * been received. As soon as a Sync Request sequence is complete, the appropriate
 * Sync Response (Sync and Follow Up Message) will be generated and sent.
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

#ifndef INCLUDE_FLASHPTP_SERVER_SERVERMODE_H_
#define INCLUDE_FLASHPTP_SERVER_SERVERMODE_H_

#include <flashptp/common/mode.h>
#include <flashptp/server/listener.h>
#include <flashptp/server/request.h>

#define FLASH_PTP_JSON_CFG_SERVER_MODE                              "serverMode"

#define FLASH_PTP_JSON_CFG_SERVER_MODE_ENABLED                      "enabled"

#define FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_1                   "priority1"
#define FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_CLASS                  "clockClass"
#define FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_ACCURACY               "clockAccuracy"
#define FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_VARIANCE               "clockVariance"
#define FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_2                   "priority2"

#define FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS                    "listeners"

namespace flashptp {

class FlashPTP;

namespace server {

class ServerMode : public Mode {
public:
    inline ServerMode(FlashPTP *flashPTP) : Mode{ flashPTP } { }
    inline ~ServerMode() { clearRequests(); clearListeners(); }

    static bool validateConfig(const Json &config, std::vector<std::string> *errs);
    bool setConfig(const Json &config, std::vector<std::string> *errs = nullptr);

    /*
     * Process incoming parts of Sync Request sequences and generate and transmit
     * the appropriate Sync Response on complete receipt.
     */
    virtual void onMsgReceived(PTP2Message *msg, int len, const struct sockaddr_storage *srcSockaddr,
            const struct sockaddr_storage *dstSockaddr, PTPTimestampLevel timestampLevel,
            const struct timespec *timestamp);

protected:
    const char *threadName() const { return "Server Mode"; }
    void threadFunc();

private:
    inline void clearListeners() { while (_listeners.size()) { delete _listeners.back(); _listeners.pop_back(); } }

    inline void clearRequests()
    {
        std::lock_guard<std::mutex> lg(_mutex);
        while (_requests.size()) {
            delete _requests.back();
            _requests.pop_back();
        }
    }
    /*
     * Find (or create) a Request object to store the received part of a Sync Request sequence in.
     * This function must be called with locked _mutex, @see onMsgReceived.
     */
    Request *obtainRequest(PTP2Message *msg, FlashPTPReqTLV *tlv,
            const struct sockaddr_storage *srcSockaddr, const struct sockaddr_storage *dstSockaddr,
            PTPTimestampLevel timestampLevel, const struct timespec *timestamp);

    /*
     * Process a complete Sync Request sequence by generating and transmitting a Sync Response.
     * This function must be called with locked _mutex, @see onMsgReceived.
     */
    void processRequest(Request *req);
    void checkRequestTimeouts();

    BMCAComparisonDataSet _bmcaComparisonDS {
        FLASH_PTP_DEFAULT_PRIORITY_1,
        FLASH_PTP_DEFAULT_CLOCK_CLASS,
        FLASH_PTP_DEFAULT_CLOCK_ACCURACY,
        FLASH_PTP_DEFAULT_CLOCK_VARIANCE,
        FLASH_PTP_DEFAULT_PRIORITY_2,
        nullptr,
        FLASH_PTP_DEFAULT_STEPS_REMOVED
    };

    std::vector<Listener*> _listeners;

    std::mutex _mutex;
    // The below vector and response buffer need to be protected by above _mutex
    std::vector<Request*> _requests;
    uint8_t _respbuf[1024];
};

}
}

#endif /* INCLUDE_FLASHPTP_SERVER_SERVERMODE_H_ */
