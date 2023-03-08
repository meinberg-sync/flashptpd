/*
 * @file server.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class Server represents the connection to another instance of flashPTP
 * running in server mode as part of the client mode for local clock synchronization.
 * Its worker thread periodically (depending on the configured interval) sends
 * Sync Request Messages and stores the appropriate sequence ID and timestamp
 * information for later evaluation of received Sync Responses.
 *
 * On receipt of a Sync Response (processed by the ClientMode worker thread),
 * a server adds the complete Request/Response sequence to a potentially configured
 * filter, adds the filter output to the calculation algorithm and calculates
 * delay, offset, drift and standard deviation as soon as the calculation algorithm
 * is ready (@see Calculation::fullyLoaded). It also records the number of completed
 * or timed out sequences within a 16-bit shift register (reach).
 *
 * Please note, that if you want to use BTCA selection algorithm, you need to enable
 * periodical requests for FlashPTPServerStateDS by setting the appropriate config
 * property "stateInterval" to the interval (2^n) in which the client shall attach
 * a request for the data set to a Sync Request sequence. "stateInterval" must be
 * equal to or bigger than "interval" (or "requestInterval").
 *
 * Examples:    0x7f = Never request FlashPTPServerStateDataSet
 *              0 = Request FlashPTPServerStateDataSet once per second
 *              3 = Request FlashPTPServerStateDataSet every eighth second
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

#ifndef INCLUDE_FLASHPTP_CLIENT_SERVER_H_
#define INCLUDE_FLASHPTP_CLIENT_SERVER_H_

#include <flashptp/common/thread.h>
#include <flashptp/network/socket.h>
#include <flashptp/client/sequence.h>
#include <flashptp/filter/filter.h>
#include <flashptp/calculation/calculation.h>

#define FLASH_PTP_CLIENT_MODE_SERVER                                    "Server"

#define FLASH_PTP_CLIENT_MODE_SERVER_OFFSET_HISTORY_SIZE                16

#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_STATE                    2
#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_SERVER                   18
#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_CLOCK                    11
#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_BTCA                     28
#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_REACH                    9
#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_INTV                     7
#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_DELAY                    13
#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_OFFSET                   13
#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_STD_DEV                  13
#define FLASH_PTP_CLIENT_MODE_SERVER_STATS_LEN                          FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_STATE + \
                                                                        FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_SERVER + \
                                                                        FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_CLOCK + \
                                                                        FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_BTCA + \
                                                                        FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_INTV + \
                                                                        FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_REACH + \
                                                                        FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_DELAY + \
                                                                        FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_OFFSET + \
                                                                        FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_STD_DEV

#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_ADDRESS               "dstAddress"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_PORT                  "dstPort"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_EVENT_PORT            "dstEventPort"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_GENERAL_PORT          "dstGeneralPort"

#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_INTERFACE             "srcInterface"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_PORT                  "srcPort"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_EVENT_PORT            "srcEventPort"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_GENERAL_PORT          "srcGeneralPort"

#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_ONE_STEP                  "oneStep"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SYNC_TLV                  "syncTLV"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_REQUEST_INTERVAL          "requestInterval"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_INTERVAL                  "interval"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_STATE_INTERVAL            "stateInterval"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_MS_TIMEOUT                "msTimeout"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_NO_SELECT                 "noSelect"

#define FLASH_PTP_JSON_CFG_SERVER_MODE_SERVER_TIMESTAMP_LEVEL           "timestampLevel"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS                   "filters"
#define FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_CALCULATION               "calculation"

namespace flashptp {
namespace client {

class ClientMode;

enum class ServerState
{
    initializing = 0,
    unreachable,
    collecting,
    ready,
    falseticker,
    candidate,
    selected
};

class Server : public Thread {
public:
    Server(const Json &config);
    ~Server();

    static const char *stateToStr(ServerState s);
    static const char *stateToLongStr(ServerState s);

    static bool validateConfig(const Json &config, std::vector<std::string> *errs);

    std::vector<network::SocketSpecs> specs() const;

    inline bool invalid() const { return _invalid; }
    inline const network::Address &dstAddress() const { return _dstAddress; }
    inline calculation::Calculation *calculation() { return _calculation; }
    inline bool noSelect() const { return _noSelect; }

    inline ServerState state() const { std::shared_lock sl(_mutex); return _state; }
    inline void setState(ServerState state) { std::unique_lock ul(_mutex); _state = state; }

    inline bool serverStateDSValid() const
    { std::shared_lock sl(_mutex); return _serverStateDSValid; }
    inline FlashPTPServerStateDS serverStateDS() const
    { std::shared_lock sl(_mutex); return _serverStateDS; }

    std::string clockName() const;
    clockid_t clockID() const;

    inline int64_t stdDev() const { std::shared_lock sl(_mutex); return _stdDev; }

    /*
     * Process incoming parts of Sync Response sequences by searching for the stored Sync Request information.
     * Call onSequenceComplete if all parts of a Sync Response have been received or onSequenceTimeout if
     * a Sync Request sequence has timed out.
     */
    Sequence *processMessage(PTP2Message *msg, FlashPTPRespTLV *tlv, PTPTimestampLevel timestampLevel,
            const struct timespec *timestamp);

    std::string printState() const;

protected:
    bool alwaysEnabled() const { return true; }
    const char *threadName() const { return _threadName.c_str(); }
    void threadFunc();

private:
    bool setConfig(const Json &config);

    /*
     * Calculate the standard deviation of the measured offsets that have been stored in stdDevHistory.
     * Use all values that are not equal to INT64_MAX, as INT64_MAX is being used for uninitialized
     * or timed out sequences.
     */
    void calcStdDev();
    /*
     * Increase reach value (shift left by one, set LSB to 1).
     *
     * If filter algorithms are to be used, add the completed sequence to the filter(s) and check,
     * if a new filter output is available. If so, add the filter output to the calculation algorithm.
     * If no filters are configured, directly add the sequence to the calculation.
     *
     * Let the calculation algorithm do its job and update the standard deviation and
     * the server state, if required.
     */
    void onSequenceComplete(Sequence *seq);
    /*
     * Decrease reach value (shift left by one, set LSB to 0).
     *
     * If no filters are to be used or the configured filters are already empty (clear them,
     * if four consecutive sequences have timed out), remove the oldest sequence from the calculation
     * algorithm. Update the standard deviation and the server state, if required.
     */
    void onSequenceTimeout(Sequence *seq);

    void resetState();

    inline void setClock(const std::string &name, clockid_t id)
    { std::unique_lock ul(_mutex); _clockName = name; _clockID = id; }

    inline void addSequence(Sequence *seq) { std::unique_lock ul(_mutex); _sequences.push_back(seq); }
    void checkSequenceTimeouts();

    std::string _threadName;

    bool _invalid{ false };

    std::string _srcInterface;
    uint16_t _srcEventPort{ FLASH_PTP_UDP_EVENT_PORT };
    uint16_t _srcGeneralPort{ FLASH_PTP_UDP_GENERAL_PORT };

    network::Address _dstAddress;
    uint16_t _dstEventPort{ FLASH_PTP_UDP_EVENT_PORT };
    uint16_t _dstGeneralPort{ FLASH_PTP_UDP_GENERAL_PORT };

    bool _oneStep{ false };
    bool _syncTLV{ false };
    int8_t _interval{ FLASH_PTP_DEFAULT_INTERVAL };
    int8_t _stateInterval{ FLASH_PTP_DEFAULT_STATE_INTERVAL };
    uint32_t _msTimeout{ FLASH_PTP_DEFAULT_TIMEOUT_MS };
    bool _noSelect{ false };

    PTPTimestampLevel _timestampLevel{ PTPTimestampLevel::hardware };

    std::vector<filter::Filter*> _filters;
    calculation::Calculation *_calculation{ nullptr };

    mutable std::shared_mutex _mutex;
    // The following members need to be read/write protected by the above mutex

    ServerState _state{ ServerState::initializing };
    uint16_t _reach{ 0 };

    bool _serverStateDSValid{ false };
    FlashPTPServerStateDS _serverStateDS;

    std::string _clockName;
    clockid_t _clockID{ -1 };

    std::vector<Sequence*> _sequences;

    int64_t _stdDevHistory[FLASH_PTP_CLIENT_MODE_SERVER_OFFSET_HISTORY_SIZE]{ INT64_MAX };
    uint8_t _stdDevIndex{ 0 };
    int64_t _stdDev{ INT64_MAX };
};

}
}

#endif /* INCLUDE_FLASHPTP_CLIENT_SERVER_H_ */
