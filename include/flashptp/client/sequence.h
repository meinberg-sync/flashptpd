/*
 * @file sequence.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class Sequence helps to store Sync Request/Sync Response information like
 * sequence ID, timestamps, timestamp level, etc. for a server connection
 * (@see client::Server) within the client mode of flashPTP.
 *
 * It also calculates the measured mean path delay and offset as soon as all
 * timestamps (T1 to T4) and corrections have been gathered.
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

#ifndef INCLUDE_FLASHPTP_CLIENT_SEQUENCE_H_
#define INCLUDE_FLASHPTP_CLIENT_SEQUENCE_H_

#include <flashptp/common/defines.h>
#include <flashptp/network/address.h>

namespace flashptp {
namespace client {

class Server;

class Sequence
{
public:
    inline Sequence(const std::string &srcInterface, uint16_t srcEventPort, uint16_t srcGeneralPort,
            const network::Address &dstAddress, uint32_t msTimeout, uint16_t sequenceID,
            PTPTimestampLevel timestampLevel, const struct timespec *timestamp, bool serverStateDSRequested)
    {
        clock_gettime(CLOCK_MONOTONIC, &_ts);

        _srcInterface = srcInterface;
        _srcEventPort = srcEventPort;
        _srcGeneralPort = srcGeneralPort;
        _dstAddress = dstAddress;
        _sequenceID = sequenceID;
        _msTimeout = msTimeout;

        _timestampLevel = timestampLevel;
        _t1 = *timestamp;
        _serverStateDSRequested = serverStateDSRequested;
    }

    inline const struct timespec &timestamp() const { return _ts; }

    inline const std::string &srcInterface() const { return _srcInterface; }
    inline uint16_t srcEventPort() const { return _srcEventPort; }
    inline uint16_t srcGeneralPort() const { return _srcGeneralPort; }
    inline const network::Address &dstAddress() const { return _dstAddress; }
    inline uint16_t sequenceID() const { return _sequenceID; }

    inline PTPTimestampLevel timestampLevel() const { return _timestampLevel; }

    inline bool timedOut() const
    {
        if (_timedOut)
            return true;

        struct timespec ts;
        int64_t diff = 0;
        bool timedOut;

        clock_gettime(CLOCK_MONOTONIC, &ts);
        diff = (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
        diff -= ((int64_t)_ts.tv_sec * 1000000000LL + (int64_t)_ts.tv_nsec);

        // Request timed out, if Sync Response has not been received two seconds after Sync Request has been sent
        _timedOut = diff > (_msTimeout * 1000000LL);
        return _timedOut;
    }

    inline bool matches(const struct sockaddr_storage *saddr, uint16_t sequenceID)
    { return _dstAddress.equals(saddr) && _sequenceID == sequenceID; }

    // Add the information included in the received Sync or Follow Up Message (and TLV) to the stored sequence.
    inline void merge(PTP2Message *msg, FlashPTPRespTLV *tlv,
            PTPTimestampLevel timestampLevel = PTPTimestampLevel::invalid, const struct timespec *timestamp = nullptr)
    {
        if (msg->msgType == (uint8_t)PTPMessageType::sync &&
            timestampLevel != PTPTimestampLevel::invalid && timestamp) {
            if (!msg->flags.twoStep)
                _t3 = msg->timestamp;
            _timestampLevel = timestampLevel;
            _t4 = *timestamp;
            _syncCorrection = msg->correction;
        }
        else if (msg->msgType == (uint8_t)PTPMessageType::followUp) {
            _t3 = msg->timestamp;
            _followUpCorrection = msg->correction;
        }
        else
            return;

        if (tlv->valid) {
            _error = *tlv->error;
            _t2 = *tlv->reqIngressTimestamp;
            _t2Correction = *tlv->reqCorrectionField;
            if (msg->flags.utcReasonable)
                _utcCorrection = (int64_t)*tlv->utcOffset * 1000000000LL;
            if (tlv->serverStateDS) {
                _serverStateDSValid = true;
                _serverStateDS = *tlv->serverStateDS;
            }
        }

        if (complete()) {
            _t4Correction = _syncCorrection;
            _t4Correction += _followUpCorrection;
        }
    }

    inline bool hasT1() const { return !_t1.empty(); }
    inline bool hasT2() const { return !_t2.empty(); }
    inline bool hasT3() const { return !_t3.empty(); }
    inline bool hasT4() const { return !_t4.empty(); }

    // Indicates, whether all needed timestamps (T1-T4) for delay and offset calculation have been received.
    inline bool complete() const
    {
        if (_complete)
            return true;
        _complete = hasT1() && hasT2() && hasT3() && hasT4();
        return _complete;
    }

    // Finish the sequence by calculating delay and offset using the collected timestamps and correction values.
    inline void finish()
    {
        _c2sDelay = _t2 - _t1 - _t2Correction - _utcCorrection;
        _s2cDelay = _t4 - _t3 - _t4Correction + _utcCorrection;
        _offset = ((_t2 + _t3 - _t2Correction - _utcCorrection) -
                (_t1 + _t4 - _t4Correction - _utcCorrection)) / 2;
    }

    inline bool hasError() const { return _error != 0; }
    inline bool hasTxTimestampError() const { return _error & FLASH_PTP_ERROR_TX_TIMESTAMP_INVALID; }

    inline bool serverStateDSRequested() const { return _serverStateDSRequested; }
    inline bool serverStateDSValid() const { return _serverStateDSValid; }
    inline const FlashPTPServerStateDS &serverStateDS() const { return _serverStateDS; }

    inline const PTP2Timestamp &t1() const { return _t1; }

    inline int64_t c2sDelay() const { return _c2sDelay; }
    inline int64_t s2cDelay() const { return _s2cDelay; }
    inline int64_t meanPathDelay() const { return (_c2sDelay + _s2cDelay) / 2; }
    inline int64_t offset() const { return _offset; }

private:
    struct timespec _ts;
    mutable bool _timedOut{ false };
    mutable bool _complete{ false };

    std::string _srcInterface;
    uint16_t _srcEventPort;
    uint16_t _srcGeneralPort;
    network::Address _dstAddress;
    uint16_t _sequenceID;
    uint32_t _msTimeout;

    PTPTimestampLevel _timestampLevel{ PTPTimestampLevel::invalid };

    PTP2Timestamp _t1;
    PTP2Timestamp _t2;
    PTP2TimeInterval _t2Correction;

    PTP2Timestamp _t3;
    PTP2Timestamp _t4;
    PTP2TimeInterval _syncCorrection;
    PTP2TimeInterval _followUpCorrection;
    PTP2TimeInterval _t4Correction;

    uint16_t _error{ 0 };

    int64_t _utcCorrection{ 0 };
    bool _serverStateDSRequested{ false };
    bool _serverStateDSValid{ false };
    FlashPTPServerStateDS _serverStateDS;

    int64_t _c2sDelay{ 0 };
    int64_t _s2cDelay{ 0 };
    int64_t _offset{ 0 };
};

}
}

#endif /* INCLUDE_FLASHPTP_CLIENT_SEQUENCE_H_ */
