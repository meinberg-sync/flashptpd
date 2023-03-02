/*
 * @file listener.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class Listener provides a worker thread, which listens (@see network::recv)
 * for incoming flashPTP Sync Requests on one of the local physical network interfaces.
 * The name of the interface, as well as the event (timestamped packets) and
 * general (non-timestamped packets) source UDP ports can be configured.
 *
 * On receipt of a part of a Sync Request sequence (Sync or Follow Up Message),
 * the onMsgReceived method of the "parent" ServerMode object is being called,
 * so that a Sync Response can be generated and transmitted as soon as a
 * Request sequence has been completely received.
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

#ifndef INCLUDE_FLASHPTP_SERVER_LISTENER_H_
#define INCLUDE_FLASHPTP_SERVER_LISTENER_H_

#include <flashptp/common/thread.h>

#define FLASH_PTP_SERVER_MODE_LISTENER                              "Listener"

#define FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_INTERFACE           "interface"
#define FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_PORT                "port"
#define FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_EVENT_PORT          "eventPort"
#define FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_GENERAL_PORT        "generalPort"

#define FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_TIMESTAMP_LEVEL     "timestampLevel"
#define FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_UTC_OFFSET          "utcOffset"

namespace flashptp {
namespace server {

class ServerMode;

class Listener: public Thread {
public:
    Listener(ServerMode *serverMode, const Json &config);

    static bool validateConfig(const Json &config, std::vector<std::string> *errs);

    inline bool invalid() const { return _invalid; }
    inline const std::string &interface() const { return _interface; }
    inline int16_t utcOffset() const { return _utcOffset; }

protected:
    bool alwaysEnabled() const { return true; }
    const char *threadName() const { return _threadName.c_str(); }
    void threadFunc();

private:
    bool setConfig(const Json &config);

    ServerMode *_serverMode;
    std::string _threadName;

    bool _invalid{ false };
    std::string _interface;
    uint16_t _eventPort{ FLASH_PTP_UDP_EVENT_PORT };
    uint16_t _generalPort{ FLASH_PTP_UDP_GENERAL_PORT };

    PTPTimestampLevel _timestampLevel{ PTPTimestampLevel::hardware };
    int16_t _utcOffset{ FLASH_PTP_DEFAULT_UTC_OFFSET };
};

}
}

#endif /* INCLUDE_FLASHPTP_SERVER_LISTENER_H_ */
