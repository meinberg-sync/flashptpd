/*
 * @file request.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class Request helps to store received parts of Sync Request sequences
 * within flashPTP server mode in order to generate and transmit appropriate
 * Sync Responses containing the stored information.
 *
 * If not all parts (Sync Message, Follow Up Message, Request TLV) of a complete
 * Sync Request sequence are received within two seconds, the Sync Request times
 * out and a Sync Response will not be sent.
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

#ifndef INCLUDE_FLASHPTP_SERVER_REQUEST_H_
#define INCLUDE_FLASHPTP_SERVER_REQUEST_H_

#include <flashptp/common/defines.h>
#include <flashptp/network/address.h>

namespace flashptp {
namespace server {

class Request {
public:
    inline Request(PTP2Message *msg, FlashPTPReqTLV *tlv, const struct sockaddr_storage *srcSockaddr,
            const struct sockaddr_storage *dstSockaddr, PTPTimestampLevel timestampLevel,
            const struct timespec *timestamp)
    {
        clock_gettime(CLOCK_MONOTONIC, &_ts);

        _srcAddress = network::Address(srcSockaddr);
        _dstAddress = network::Address(dstSockaddr);
        _sequenceID = msg->seqID;

        merge(msg, tlv, srcSockaddr, dstSockaddr, timestampLevel, timestamp);
    }

    inline const timespec &ts() const { return _ts; }

    inline const network::Address &srcAddress() const { return _srcAddress; }
    inline uint16_t srcEventPort() const { return _srcEventPort; }
    inline uint16_t srcGeneralPort() const { return _srcGeneralPort; }
    inline const network::Address &dstAddress() const { return _dstAddress; }
    inline uint16_t dstEventPort() const { return _dstEventPort; }
    inline uint16_t dstGeneralPort() const { return _dstGeneralPort; }
    inline uint16_t sequenceID() const { return _sequenceID; }

    inline const PTP2TimeInterval &correction() const { return _correction; }
    inline PTPVersion ptpVersion() const { return _ptpVersion; }
    inline PTPTimestampLevel timestampLevel() const { return _timestampLevel; }
    inline const PTP2Timestamp &ingressTimestamp() const { return _ingressTimestamp; }

    inline uint32_t flags() const { return _flags; }
    inline bool syncTLV() const { return _syncTLV; }
    inline bool oneStep() const { return _oneStep; }

    inline bool timedOut() const
    {
        struct timespec ts;
        int64_t diff = 0;

        clock_gettime(CLOCK_MONOTONIC, &ts);
        diff = (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
        diff -= ((int64_t)_ts.tv_sec * 1000000000LL + (int64_t)_ts.tv_nsec);

        // Request timed out, if Follow Up has not been received two seconds after Sync Request
        return diff > (FLASH_PTP_DEFAULT_TIMEOUT_MS * 1000000LL);
    }

    inline bool matches(const struct sockaddr_storage *saddr, uint16_t sequenceID)
    {
        return _srcAddress.equals(saddr) &&
               _sequenceID == sequenceID;
    }

    // Add the information included in the received Sync or Follow Up Message (and TLV) to the stored request.
    inline void merge(PTP2Message *msg, FlashPTPReqTLV *tlv, const struct sockaddr_storage *srcSockaddr,
            const struct sockaddr_storage *dstSockaddr, PTPTimestampLevel timestampLevel = PTPTimestampLevel::invalid,
            const struct timespec *timestamp = nullptr)
    {
        uint16_t srcPort, dstPort;

        switch (srcSockaddr->ss_family) {
        case AF_INET: srcPort = ntohs(((struct sockaddr_in*)srcSockaddr)->sin_port); break;
        case AF_INET6: srcPort = ntohs(((struct sockaddr_in6*)srcSockaddr)->sin6_port); break;
        default: srcPort = 0; break;
        }

        switch (dstSockaddr->ss_family) {
        case AF_INET: dstPort = ntohs(((struct sockaddr_in*)dstSockaddr)->sin_port); break;
        case AF_INET6: dstPort = ntohs(((struct sockaddr_in6*)dstSockaddr)->sin6_port); break;
        default: dstPort = 0; break;
        }

        if (msg->msgType == (uint8_t)PTPMessageType::sync &&
            timestampLevel != PTPTimestampLevel::invalid && timestamp) {
            _srcEventPort = srcPort;
            _dstEventPort = dstPort;
            _ptpVersion = (PTPVersion)msg->version;
            _timestampLevel = timestampLevel;
            _ingressTimestamp = *timestamp;
            _oneStep = !msg->flags.twoStep;
            _syncCorrection = msg->correction;
            _syncTLV = tlv->valid;
            _syncReceived = true;
        }
        else if (msg->msgType == (uint8_t)PTPMessageType::followUp) {
            _srcGeneralPort = srcPort;
            _dstGeneralPort = dstPort;
            _followUpCorrection = msg->correction;
            _followUpReceived = true;
        }
        else
            return;

        if (tlv->valid && !_tlvReceived) {
            _flags = tlv->hdr->flags;
            _tlvReceived = true;
        }

        if (complete()) {
            _correction = _syncCorrection;
            _correction += _followUpCorrection;
        }
    }

    inline bool syncReceived() const { return _syncReceived; }
    inline bool followUpReceived() const { return _followUpReceived; }
    inline bool tlvReceived() const { return _tlvReceived; }

    /*
     * Indicates, whether all parts of a Sync Request sequence (i.e., Sync and Follow Up Message
     * and Request TLV) have been received.
     */
    inline bool complete() const { return syncReceived() && (oneStep() || followUpReceived()) && tlvReceived(); }

private:
    struct timespec _ts;

    network::Address _srcAddress;
    uint16_t _srcEventPort;
    uint16_t _srcGeneralPort;
    network::Address _dstAddress;
    uint16_t _dstEventPort;
    uint16_t _dstGeneralPort;
    uint16_t _sequenceID;

    PTP2TimeInterval _syncCorrection;
    PTP2TimeInterval _followUpCorrection;
    PTP2TimeInterval _correction;

    PTPVersion _ptpVersion{ PTPVersion::invalid };
    PTPTimestampLevel _timestampLevel{ PTPTimestampLevel::invalid };
    PTP2Timestamp _ingressTimestamp;

    uint32_t _flags;
    bool _syncTLV;
    bool _oneStep;

    bool _syncReceived{ false };
    bool _followUpReceived{ false };
    bool _tlvReceived{ false };
};

}
}

#endif /* INCLUDE_FLASHPTP_SERVER_REQUEST_H_ */
