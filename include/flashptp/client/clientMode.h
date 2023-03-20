/*
 * @file clientMode.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class ClientMode implements the abstract class Mode and represents the
 * client mode within flashPTP. Its worker thread starts all configured servers
 * (their worker threads), listens on incoming flashPTP Sync Response messages
 * and processes them to the corresponding server. It also checks for
 * availability of clock adjustments, selects the best servers and performs
 * clock adjustments accordingly.
 *
 * While only one server selection algorithm can be specified, multiple adjustment
 * algorithms can be configured for the different clocks that might be available
 * within the system running flashPTP. Therefore, you can run software timestamp
 * based measurements synchronizing the system clock (CLOCK_REALTIME), while running
 * hardware timestamp based synchronization of one or multiple PHCs (/dev/ptp0, etc.)
 * at the same time. You could also synchronize only one PHC and use a tool like
 * phc2sys to adjust the system clock. Adjusting other clocks within the system
 * based on the configured clock adjustments of the client mode is not part of
 * flashPTP, yet.
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

#ifndef INCLUDE_FLASHPTP_CLIENT_CLIENTMODE_H_
#define INCLUDE_FLASHPTP_CLIENT_CLIENTMODE_H_

#include <flashptp/client/server.h>
#include <flashptp/selection/selection.h>
#include <flashptp/adjustment/adjustment.h>
#include <flashptp/common/mode.h>

#define FLASH_PTP_JSON_CFG_CLIENT_MODE                  "clientMode"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_ENABLED          "enabled"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS          "servers"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION        "selection"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS      "adjustments"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_STATE_FILE       "stateFile"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_STATE_TABLE      "stateTable"

namespace flashptp {

namespace client {

class ClientMode : public Mode {
public:
    inline ClientMode(FlashPTP *flashPTP) : Mode{ flashPTP } { }
    inline ~ClientMode() { clearServers(); if (_selection) delete _selection; clearAdjustments(); }

    static bool validateConfig(const Json &config, std::vector<std::string> *errs);
    bool setConfig(const Json &config, std::vector<std::string> *errs = nullptr);

    // Process incoming parts of Sync Response sequences to the appropriate servers
    virtual void onMsgReceived(PTP2Message *msg, int len, const struct sockaddr_storage *srcSockaddr,
            const struct sockaddr_storage *dstSockaddr, PTPTimestampLevel timestampLevel,
            const struct timespec *timestamp);

protected:
    const char *threadName() const { return "Client Mode"; }
    void threadFunc();

private:
    void resetUnusedServersStates();
    void printState();
    inline void clearServers()
    {
        while (_servers.size()) {
            delete _servers.back();
            _servers.pop_back();
        }
    }

    // Indicates, whether a new adjustment is available for the provided clock id
    bool hasAdjustment(clockid_t id) const;
    /*
     * Checks for availability of new adjustments for all configured adjustment algorithms,
     * uses the configured selection algorithm for the selection of servers and applies the
     * adjustments accordingly.
     */
    void performAdjustments();
    inline void clearAdjustments()
    {
        while (_adjustments.size()) {
            delete _adjustments.back();
            _adjustments.pop_back();
        }
    }

    std::string _stateFile;
    bool _stateTable{ false };
    unsigned _stateTableRows{ 0 };

    std::vector<Server*> _servers;

    selection::Selection *_selection{ nullptr };
    std::vector<adjustment::Adjustment*> _adjustments;
};

}
}

#endif /* INCLUDE_FLASHPTP_CLIENT_CLIENTMODE_H_ */
