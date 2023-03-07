/*
 * @file defines.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * This file provides globally used enum classes, struct definitons, defines
 * and conversion functions for flashPTP.
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

#ifndef INCLUDE_FLASHPTP_COMMON_DEFINES_H_
#define INCLUDE_FLASHPTP_COMMON_DEFINES_H_

#include <flashptp/common/includes.h>

namespace flashptp {

enum class PTPProtocol
{
    invalid = 0,
    ip4,
    ip6,
    ieee_802_3
};

inline static const char *ptpProtocolToStr(PTPProtocol p)
{
    switch (p) {
    case PTPProtocol::ip4: return "IPv4";
    case PTPProtocol::ip6: return "IPv6";
    case PTPProtocol::ieee_802_3: return "IEEE 802.3";
    default: return "Invalid";
    }
}

inline static unsigned ptpProtocolToAddrLen(PTPProtocol p)
{
    switch (p) {
    case PTPProtocol::ip4: return 4;
    case PTPProtocol::ip6: return 16;
    case PTPProtocol::ieee_802_3: return 6;
    default: return 0;
    }
}

inline static unsigned ptpProtocolToFamily(PTPProtocol p)
{
    switch (p) {
    case PTPProtocol::ip4: return AF_INET;
    case PTPProtocol::ip6: return AF_INET6;
    case PTPProtocol::ieee_802_3: return AF_PACKET;
    default: return AF_UNSPEC;
    }
}

inline static PTPProtocol ptpProtocolFromFamily(unsigned f)
{
    switch (f) {
    case AF_INET: return PTPProtocol::ip4;
    case AF_INET6: return PTPProtocol::ip6;
    case AF_PACKET: return PTPProtocol::ieee_802_3;
    default: return PTPProtocol::invalid;
    }
}

enum class PTPVersion
{
    v1 = 1,
    v2_0,
    v2_1 = 0x12
};

inline static const char *ptpVersionToStr(PTPVersion v)
{
    switch (v) {
    case PTPVersion::v1: return "PTPv1";
    case PTPVersion::v2_0: return "PTPv2";
    case PTPVersion::v2_1: return "PTPv2.1";
    default: return "Invalid";
    }
}

enum class PTPMessageType
{
    sync = 0,
    delayReq,
    pDelayReq,
    pDelayResp,
    followUp = 8,
    delayResp,
    pDelayRespFollowUp,
    announce,
    signalling,
    management
};

inline static const char *ptpMessageTypeToStr(PTPMessageType t)
{
    switch (t) {
    case PTPMessageType::sync: return "Sync";
    case PTPMessageType::delayReq: return "Delay Request";
    case PTPMessageType::pDelayReq: return "Peer Delay Request";
    case PTPMessageType::pDelayResp: return "Peer Delay Response";
    case PTPMessageType::followUp: return "Follow Up";
    case PTPMessageType::delayResp: return "Delay Response";
    case PTPMessageType::pDelayRespFollowUp: return "Peer Delay Response Follow Up";
    case PTPMessageType::announce: return "Announce";
    case PTPMessageType::signalling: return "Signalling";
    case PTPMessageType::management: return "Management";
    default: return "Invalid";
    }
}

enum class PTPMessageControl
{
    sync = 0,
    delayReq,
    followUp,
    delayResp,
    management,
    other
};

enum class PTPTimestampLevel
{
    invalid,
    user,
    socket,
    hardware,
    max = hardware
};

inline static const char *ptpTimestampLevelToShortStr(PTPTimestampLevel l)
{
    switch (l) {
    case PTPTimestampLevel::invalid: return "no";
    case PTPTimestampLevel::user: return "usr";
    case PTPTimestampLevel::socket: return "so";
    case PTPTimestampLevel::hardware: return "hw";
    default: return "inv";
    }
}

inline static const char *ptpTimestampLevelToStr(PTPTimestampLevel l)
{
    switch (l) {
    case PTPTimestampLevel::invalid: return "No";
    case PTPTimestampLevel::user: return "User-Level";
    case PTPTimestampLevel::socket: return "Socket";
    case PTPTimestampLevel::hardware: return "Hardware";
    default: return "Invalid";
    }
}

inline PTPTimestampLevel ptpTimestampLevelFromShortStr(const char *str)
{
    for (int i = (int)PTPTimestampLevel::user; i <= (int)PTPTimestampLevel::hardware; ++i) {
        if (strcmp(ptpTimestampLevelToShortStr((PTPTimestampLevel)i), str) == 0)
            return (PTPTimestampLevel)i;
    }
    return PTPTimestampLevel::invalid;
}

#define FLASH_PTP_UDP_EVENT_PORT                319
#define FLASH_PTP_UDP_GENERAL_PORT              320

#define FLASH_PTP_SYSTEM_CLOCK_NAME             "system"

#define FLASH_PTP_FIXED_VERSION                 PTPVersion::v2_1
#define FLASH_PTP_FIXED_SDO_ID                  0x000
#define FLASH_PTP_FIXED_DOMAIN_NUMBER           0

#define FLASH_PTP_DEFAULT_INTERVAL              0
#define FLASH_PTP_DEFAULT_STATE_INTERVAL        0x7f

#define FLASH_PTP_DEFAULT_FILTER_SIZE           16
#define FLASH_PTP_DEFAULT_FILTER_PICK           1

#define FLASH_PTP_DEFAULT_CALCULATION_SIZE      8

#define FLASH_PTP_DEFAULT_SELECTION_PICK        1

#define FLASH_PTP_DEFAULT_UTC_OFFSET            37
#define FLASH_PTP_DEFAULT_PRIORITY_1            128
#define FLASH_PTP_DEFAULT_CLOCK_CLASS           248
#define FLASH_PTP_DEFAULT_CLOCK_ACCURACY        0x2f
#define FLASH_PTP_DEFAULT_CLOCK_VARIANCE        65535
#define FLASH_PTP_DEFAULT_PRIORITY_2            128
#define FLASH_PTP_DEFAULT_STEPS_REMOVED         0
#define FLASH_PTP_DEFAULT_TIME_SOURCE           0x60

#define FLASH_PTP_DEFAULT_TIMEOUT_MS            2000

#define FLASH_PTP_ORG_EXT_TLV                   3

#define FLASH_PTP_MEINBERG_ORG_ID_0             0xEC
#define FLASH_PTP_MEINBERG_ORG_ID_1             0x46
#define FLASH_PTP_MEINBERG_ORG_ID_2             0x70

#define FLASH_PTP_REQUEST_SUB_TYPE_0            0x52  // R
#define FLASH_PTP_REQUEST_SUB_TYPE_1            0x65  // e
#define FLASH_PTP_REQUEST_SUB_TYPE_2            0x71  // q

#define FLASH_PTP_RESPONSE_SUB_TYPE_0           0x52  // R
#define FLASH_PTP_RESPONSE_SUB_TYPE_1           0x65  // e
#define FLASH_PTP_RESPONSE_SUB_TYPE_2           0x73  // s

// Flags used with FlashPTPTLVHdr::flags
#define FLASH_PTP_FLAG_SERVER_STATE_DS          0x1

// Error Codes used with FlashPTPRespTLV::error
#define FLASH_PTP_ERROR_OP_MODE_NOT_SUPP        0x0001

inline static const char *flashPTPMessageTypeToStr(uint8_t *orgSubType)
{
    if (orgSubType[0] == FLASH_PTP_REQUEST_SUB_TYPE_0 &&
        orgSubType[1] == FLASH_PTP_REQUEST_SUB_TYPE_1 &&
        orgSubType[2] == FLASH_PTP_REQUEST_SUB_TYPE_2)
        return "Request";
    else if (orgSubType[0] == FLASH_PTP_RESPONSE_SUB_TYPE_0 &&
             orgSubType[1] == FLASH_PTP_RESPONSE_SUB_TYPE_1 &&
             orgSubType[2] == FLASH_PTP_RESPONSE_SUB_TYPE_2)
        return "Response";
    else
        return "Invalid";
}

inline static int64_t subtractTimespecs(const struct timespec &ts1, const struct timespec &ts2)
{
    int64_t ns1 = ((int64_t)ts1.tv_sec * 1000000000ULL) + ts1.tv_nsec;
    int64_t ns2 = ((int64_t)ts2.tv_sec * 1000000000ULL) + ts2.tv_nsec;
    return ns1 - ns2;
}

inline std::string nanosecondsToStr(int64_t ns)
{
    std::stringstream ss;
    if (ns == INT64_MAX)
        return "-";

    if (ns < 0)
        ss << "-";

    int64_t secs = llabs(ns) / 1000000000;
    int64_t nsecs = llabs(ns) % 1000000000;
    if (secs > 0) {
        ss << (int)secs;
        ss << ".";
        ss << std::setfill('0') << std::setw(3) << (int)(nsecs / 1000000);
        ss << " s";
    }
    else if (nsecs >= 1000000) {
        ss << (int)(nsecs / 1000000);
        ss << ".";
        ss << std::setfill('0') << std::setw(3) << (int)((nsecs % 1000000) / 1000);
        ss << " ms";
    }
    else if (nsecs >= 1000) {
        ss << (int)(nsecs / 1000);
        ss << ".";
        ss << std::setfill('0') << std::setw(3) << (int)(nsecs % 1000);
        ss << " us";
    }
    else {
        ss << (int)(nsecs);
        ss << " ns";
    }

    return ss.str();
}

template<typename T>
inline static std::string enumClassToStr(const char*(toStr)(T))
{
    std::string str;
    for (int i = (int)T::invalid + 1; i <= (int)T::max; ++i) {
        if (!str.empty())
            str.append("/");
        str.append(toStr((T)i));
    }
    return str;
}

inline static uint64_t swap64(uint64_t netll)
{
    uint64_t hostll;
    uint8_t *b = (uint8_t*)&hostll;
    b[0] = (uint8_t)((netll >> 56) & 0xff);
    b[1] = (uint8_t)((netll >> 48) & 0xff);
    b[2] = (uint8_t)((netll >> 40) & 0xff);
    b[3] = (uint8_t)((netll >> 32) & 0xff);
    b[4] = (uint8_t)((netll >> 24) & 0xff);
    b[5] = (uint8_t)((netll >> 16) & 0xff);
    b[6] = (uint8_t)((netll >> 8) & 0xff);
    b[7] = (uint8_t)(netll & 0xff);
    return hostll;
}

#pragma pack(push, 1)

struct PTP2Flags
{
    uint8_t alternateTimeTransmitter    : 1;
    uint8_t twoStep                     : 1;
    uint8_t unicast                     : 1;
    uint8_t reserved1                   : 1;
    uint8_t reserved2                   : 1;
    uint8_t profileSpecific1            : 1;
    uint8_t profileSpecific2            : 1;
    uint8_t security                    : 1;
    uint8_t leapIndicator61             : 1;
    uint8_t leapIndicator59             : 1;
    uint8_t utcReasonable               : 1;
    uint8_t timescale                   : 1;
    uint8_t timeTraceable               : 1;
    uint8_t frequencyTraceable          : 1;
    uint8_t reserved3                   : 1;
    uint8_t reserved4                   : 1;

    inline PTP2Flags(bool ts = true)
    {
        memset(this, 0, sizeof(*this));
        twoStep = ts;
        unicast = 1;
    }
};

struct PTP2TimeInterval
{
    int64_t scaledNanoseconds = { 0 };

    inline void reorder()
    {
        scaledNanoseconds = swap64(scaledNanoseconds);
    }

    inline int64_t nanoseconds() const
    {
        int64_t ns = (scaledNanoseconds >> 16) & 0xffffffffffff;
        if (ns < 0) {
            ns |= 0xffff000000000000;
            ns += 1;
        }
        return ns;
    }

    inline PTP2TimeInterval &operator+=(const PTP2TimeInterval &r)
    { scaledNanoseconds += r.scaledNanoseconds; return *this; }

    inline friend int64_t operator-(const int64_t &l, const PTP2TimeInterval &r)
    { return l - r.nanoseconds(); }

    inline friend int64_t operator+(const int64_t &l, const PTP2TimeInterval &r)
    { return l + r.nanoseconds(); }
};

struct PTP2Timestamp
{
    uint8_t sec[6]{ 0 };
    uint32_t ns{ 0 };

    inline void reorder(bool ntoh = true)
    {
        if (ntoh)
            ns = ntohl(ns);
        else
            ns = htonl(ns);
    }

    inline bool empty() const
    {
        return sec[0] == 0 && sec[1] == 0 && sec[2] == 0 &&
               sec[3] == 0 && sec[4] == 0 && sec[5] == 0 &&
               ns == 0;
    }

    inline void reset()
    {
        memset(sec, 0, sizeof(sec));
        ns = 0;
    }

    inline int64_t seconds() const
    {
        int64_t seconds = 0;
        for (unsigned i = 0; i < sizeof(sec); ++i)
            seconds |= ((int64_t)sec[i] << (40 - (i * 8)));
        return seconds;
    }

    inline PTP2Timestamp &operator=(const struct timespec &ts)
    {
        int64_t seconds = ts.tv_sec;
        sec[0] = (seconds >> 40) & 0xff;
        sec[1] = (seconds >> 32) & 0xff;
        sec[2] = (seconds >> 24) & 0xff;
        sec[3] = (seconds >> 16) & 0xff;
        sec[4] = (seconds >> 8) & 0xff;
        sec[5] = seconds & 0xff;
        ns = ts.tv_nsec;
        return *this;
    }

    inline operator timespec() const
    {
        struct timespec ts;
        ts.tv_sec = seconds();
        ts.tv_nsec = ns;
        return ts;
    }

    inline friend int64_t operator-(const PTP2Timestamp &l, const PTP2Timestamp &r)
    {
        if (&l == &r)
            return 0;
        else
            return (l.seconds() * 1000000000LL + (int64_t)l.ns) -
                   (r.seconds() * 1000000000LL + (int64_t)r.ns);
    }

    inline friend int64_t operator+(const PTP2Timestamp &l, const PTP2Timestamp &r)
    {
        return (l.seconds() * 1000000000LL) + (int64_t)l.ns +
               (r.seconds() * 1000000000LL) + (int64_t)r.ns;
    }
};

struct PTP2ClockID
{
    uint8_t b[8]{ 0 };

    inline bool empty() const
    {
        return b[0] == 0 && b[1] == 0 && b[2] == 0 && b[3] == 0 &&
               b[4] == 0 && b[5] == 0 && b[6] == 0 && b[7] == 0;
    }

    inline void reset() { memset(b, 0, sizeof(b)); }

    inline std::string str() const
    {
        std::stringstream sstr("0x");
        for (unsigned i = 0; i < sizeof(b); ++i)
            sstr << std::hex << std::setfill('0') << std::setw(2) << (unsigned)b[i];
        return sstr.str();
    }
};

using PTP2PortID = uint16_t;

struct PTP2PortIdentity
{
    PTP2ClockID clockID;
    PTP2PortID portID{ 0 };

    inline void reorder(bool ntoh = true)
    {
        if (ntoh)
            portID = ntohs(portID);
        else
            portID = htons(portID);
    }

    inline std::string str() const
    {
        std::stringstream sstr;
        sstr << clockID.str();
        sstr << ":";
        sstr << std::setfill('0') << std::setw(5) << (unsigned)portID;
        return sstr.str();
    }

    inline friend bool operator==(const PTP2PortIdentity &l, const PTP2PortIdentity &r)
    {
        if (&l == &r)
            return true;
        else
            return memcmp(&l.clockID, &r.clockID, sizeof(l.clockID)) == 0 &&
                    l.portID == r.portID;
    }
};

struct PTP2Message
{
    uint8_t msgType : 4;
    uint8_t sdoIDMajor : 4;
    uint8_t version;
    uint16_t totalLen;
    uint8_t domain;
    uint8_t sdoIDMinor;
    PTP2Flags flags;
    PTP2TimeInterval correction;
    uint32_t msgTypeSpecific{ 0 };
    PTP2PortIdentity portIdentity;
    uint16_t seqID{ 0 };
    uint8_t msgCtrl;
    int8_t logMsgPeriod;
    PTP2Timestamp timestamp;

    inline void reorder(bool ntoh = true)
    {
        if (ntoh) {
            totalLen = ntohs(totalLen);
            msgTypeSpecific = ntohl(msgTypeSpecific);
            seqID = ntohs(seqID);
        }
        else {
            totalLen = htons(totalLen);
            msgTypeSpecific = htonl(msgTypeSpecific);
            seqID = htons(seqID);
        }
        correction.reorder();
        portIdentity.reorder(ntoh);
        timestamp.reorder(ntoh);
    }

    inline void init(PTPMessageType type, uint16_t length, bool twoStep = false)
    {
        msgType = (uint8_t)type;
        sdoIDMajor = (FLASH_PTP_FIXED_SDO_ID >> 8) & 0xf;
        version = (uint8_t)FLASH_PTP_FIXED_VERSION;
        totalLen = length;
        domain = FLASH_PTP_FIXED_DOMAIN_NUMBER;
        sdoIDMinor = (FLASH_PTP_FIXED_SDO_ID & 0xff);
        flags = PTP2Flags(twoStep);
        if (type == PTPMessageType::sync)
            msgCtrl = (uint8_t)PTPMessageControl::sync;
        else if (type == PTPMessageType::followUp)
            msgCtrl = (uint8_t)PTPMessageControl::followUp;
        else
            msgCtrl = (uint8_t)PTPMessageControl::other;
        logMsgPeriod = 0x7f;
    }

    inline PTP2Message(PTPMessageType type, uint16_t length, bool twoStep)
    {
        init(type, length, twoStep);
    }
};

struct FlashPTPTLVHdr
{
    uint16_t tlvType{ FLASH_PTP_ORG_EXT_TLV };
    uint16_t tlvLength;
    uint8_t organizationID[3] {
        FLASH_PTP_MEINBERG_ORG_ID_0,
        FLASH_PTP_MEINBERG_ORG_ID_1,
        FLASH_PTP_MEINBERG_ORG_ID_2
    };
    uint8_t organizationSubType[3];
    uint32_t flags{ 0 };

    inline void reorder(bool ntoh = true)
    {
        if (ntoh) {
            tlvType = ntohs(tlvType);
            tlvLength = ntohs(tlvLength);
            flags = ntohl(flags);
        }
        else {
            tlvType = htons(tlvType);
            tlvLength = htons(tlvLength);
            flags = htonl(flags);
        }
    }

    inline bool isSyncRequest() const
    {
        return tlvType == FLASH_PTP_ORG_EXT_TLV &&
               tlvLength > sizeof(FlashPTPTLVHdr) &&
               organizationID[0] == FLASH_PTP_MEINBERG_ORG_ID_0 &&
               organizationID[1] == FLASH_PTP_MEINBERG_ORG_ID_1 &&
               organizationID[2] == FLASH_PTP_MEINBERG_ORG_ID_2 &&
               organizationSubType[0] == FLASH_PTP_REQUEST_SUB_TYPE_0 &&
               organizationSubType[1] == FLASH_PTP_REQUEST_SUB_TYPE_1 &&
               organizationSubType[2] == FLASH_PTP_REQUEST_SUB_TYPE_2;
    }

    inline bool isSyncResponse() const
    {
        return tlvType == FLASH_PTP_ORG_EXT_TLV &&
               tlvLength > sizeof(FlashPTPTLVHdr) &&
               organizationID[0] == FLASH_PTP_MEINBERG_ORG_ID_0 &&
               organizationID[1] == FLASH_PTP_MEINBERG_ORG_ID_1 &&
               organizationID[2] == FLASH_PTP_MEINBERG_ORG_ID_2 &&
               organizationSubType[0] == FLASH_PTP_RESPONSE_SUB_TYPE_0 &&
               organizationSubType[1] == FLASH_PTP_RESPONSE_SUB_TYPE_1 &&
               organizationSubType[2] == FLASH_PTP_RESPONSE_SUB_TYPE_2;
    }

    static inline bool validate(PTP2Message *msg, int len, bool *isRequest = nullptr)
    {
        FlashPTPTLVHdr *tlvhdr;
        bool rc = false;

        if (len < (sizeof(*msg) + sizeof(*tlvhdr)))
            return rc;

        tlvhdr = (FlashPTPTLVHdr*)((uint8_t*)msg + sizeof(*msg));
        tlvhdr->reorder(true);
        if (len >= (sizeof(*msg) + tlvhdr->tlvLength) &&
           (tlvhdr->isSyncRequest() || tlvhdr->isSyncResponse())) {
            if (isRequest)
                *isRequest = tlvhdr->isSyncRequest();
            rc = true;
        }
        tlvhdr->reorder(false);

        return rc;
    }
};

struct FlashPTPServerStateDS
{
    uint8_t gmPriority1{ 0 };
    uint8_t gmClockClass{ 0 };
    uint8_t gmClockAccuracy{ 0 };
    uint16_t gmClockVariance{ 0 };
    uint8_t gmPriority2{ 0 };
    PTP2ClockID gmClockID;
    uint16_t stepsRemoved{ 0 };
    uint8_t timeSource{ 0 };
    uint8_t reserved{ 0 };

    inline void reorder(bool ntoh = true)
    {
        if (ntoh) {
            gmClockVariance = ntohs(gmClockVariance);
            stepsRemoved = ntohs(stepsRemoved);
        }
        else {
            gmClockVariance = htons(gmClockVariance);
            stepsRemoved = htons(stepsRemoved);
        }
    }

    FlashPTPServerStateDS() = default;
    inline FlashPTPServerStateDS(uint8_t p1, uint8_t cc, uint8_t ca, uint16_t cv, uint8_t p2,
            PTP2ClockID *id, uint16_t sr, uint8_t ts)
    {
        gmPriority1 = p1;
        gmClockClass = cc;
        gmClockAccuracy = ca;
        gmClockVariance = cv;
        gmPriority2 = p2;
        if (id)
            gmClockID = *id;
        stepsRemoved = sr;
        timeSource = ts;
    }

    inline std::string toBTCAStr() const
    {
        std::stringstream sstr;
        sstr << unsigned(gmPriority1);
        sstr << "/";
        sstr << unsigned(gmClockClass);
        sstr << "/0x";
        sstr << std::hex;
        sstr << std::setfill('0') << std::setw(2) << (unsigned)(gmClockAccuracy & 0xff);
        sstr << "/0x";
        sstr << std::setfill('0') << std::setw(4) << (unsigned)(gmClockVariance & 0xffff);
        sstr << "/";
        sstr << std::dec;
        sstr << unsigned(gmPriority2);
        sstr << "/";
        sstr << unsigned(stepsRemoved);
        return sstr.str();
    }
};

struct FlashPTPReqTLV
{
    const uint8_t *buf{ nullptr };
    uint16_t buflen{ 0 };
    bool valid{ false };

    FlashPTPTLVHdr *hdr{ nullptr };
    uint8_t *pad{ nullptr };
    uint16_t padlen{ 0 };

    inline void reorder(bool ntoh = true)
    {
        if (!valid)
            return;
        hdr->reorder(ntoh);
    }

    // use this for incoming messages
    inline void rxRestore(const uint8_t *ptr, uint16_t len)
    {
        uint16_t pos = 0;

        buf = ptr;
        buflen = len;
        valid = false;

        if (!ptr)
            return;

        if (len < sizeof(*hdr))
            return;
        hdr = (FlashPTPTLVHdr*)&ptr[pos];
        len -= sizeof(*hdr);
        pos += sizeof(*hdr);

        padlen = sizeof(uint16_t) + sizeof(PTP2Timestamp) + sizeof(PTP2TimeInterval) + sizeof(int16_t);
        if (ntohl(hdr->flags) & FLASH_PTP_FLAG_SERVER_STATE_DS)
            padlen += sizeof(FlashPTPServerStateDS);
        if (len < padlen)
            return;
        pad = (uint8_t*)&ptr[pos];

        valid = true;
    }

    // use this for outgoing messages
    inline void txPrepare(const uint8_t *ptr, uint16_t len, uint32_t flags)
    {
        uint16_t pos = 0;

        buf = ptr;
        buflen = len;
        valid = false;

        if (!ptr)
            return;

        if (len < sizeof(*hdr))
            return;
        hdr = (FlashPTPTLVHdr*)&ptr[pos];
        *hdr = FlashPTPTLVHdr();
        hdr->organizationSubType[0] = FLASH_PTP_REQUEST_SUB_TYPE_0;
        hdr->organizationSubType[1] = FLASH_PTP_REQUEST_SUB_TYPE_1;
        hdr->organizationSubType[2] = FLASH_PTP_REQUEST_SUB_TYPE_2;
        hdr->flags = flags;
        len -= sizeof(*hdr);
        pos += sizeof(*hdr);

        padlen = sizeof(uint16_t) + sizeof(PTP2Timestamp) + sizeof(PTP2TimeInterval) + sizeof(int16_t);
        if (flags & FLASH_PTP_FLAG_SERVER_STATE_DS)
            padlen += sizeof(FlashPTPServerStateDS);
        if (len < padlen)
            return;
        pad = (uint8_t*)&ptr[pos];
        memset(pad, 0, padlen);

        hdr->tlvLength = sizeof(*hdr) + padlen;
        valid = true;
    }

    inline FlashPTPReqTLV() = default;
    inline uint16_t len() const { if (!valid) return 0; else return hdr->tlvLength; }
};

struct FlashPTPRespTLV
{
    const uint8_t *buf{ nullptr };
    uint16_t buflen{ 0 };
    bool valid{ false };

    FlashPTPTLVHdr *hdr{ nullptr };
    uint16_t *error{ nullptr };
    PTP2Timestamp *reqIngressTimestamp{ nullptr };
    PTP2TimeInterval *reqCorrectionField{ nullptr };
    int16_t *utcOffset{ nullptr };
    FlashPTPServerStateDS *serverStateDS{ nullptr };

    inline void reorder(bool ntoh = true)
    {
        if (!valid)
            return;
        hdr->reorder(ntoh);

        reqIngressTimestamp->reorder(ntoh);
        reqCorrectionField->reorder();
        if (ntoh) {
            *error = ntohs(*error);
            *utcOffset = ntohs(*utcOffset);
        }
        else {
            *error = htons(*error);
            *utcOffset = htons(*utcOffset);
        }
        if (serverStateDS)
            serverStateDS->reorder(ntoh);
    }

    inline void rxRestore(const uint8_t *ptr, uint16_t len)
    {
        uint16_t pos = 0;

        buf = ptr;
        buflen = len;
        valid = false;

        if (!ptr)
            return;

        if (len < sizeof(*hdr))
            return;
        hdr = (FlashPTPTLVHdr*)&ptr[pos];
        len -= sizeof(*hdr);
        pos += sizeof(*hdr);

        if (len < sizeof(*error))
            return;
        error = (uint16_t*)&ptr[pos];
        len -= sizeof(*error);
        pos += sizeof(*error);

        if (len < sizeof(*reqIngressTimestamp))
            return;
        reqIngressTimestamp = (PTP2Timestamp*)&ptr[pos];
        len -= sizeof(*reqIngressTimestamp);
        pos += sizeof(*reqIngressTimestamp);

        if (len < sizeof(*reqCorrectionField))
            return;
        reqCorrectionField = (PTP2TimeInterval*)&ptr[pos];
        len -= sizeof(*reqCorrectionField);
        pos += sizeof(*reqCorrectionField);

        if (len < sizeof(*utcOffset))
            return;
        utcOffset = (int16_t*)&ptr[pos];
        len -= sizeof(*utcOffset);
        pos += sizeof(*utcOffset);

        if (ntohl(hdr->flags) & FLASH_PTP_FLAG_SERVER_STATE_DS) {
            if (len < sizeof(*serverStateDS))
                return;
            serverStateDS = (FlashPTPServerStateDS*)&ptr[pos];
            len -= sizeof(*serverStateDS);
            pos += sizeof(*serverStateDS);
        }

        valid = true;
    }

    inline void txPrepare(const uint8_t *ptr, uint16_t len, uint32_t flags)
    {
        uint16_t pos = 0;

        buf = ptr;
        buflen = len;
        valid = false;

        if (!ptr)
            return;

        if (len < sizeof(*hdr))
            return;
        hdr = (FlashPTPTLVHdr*)&ptr[pos];
        *hdr = FlashPTPTLVHdr();
        hdr->organizationSubType[0] = FLASH_PTP_RESPONSE_SUB_TYPE_0;
        hdr->organizationSubType[1] = FLASH_PTP_RESPONSE_SUB_TYPE_1;
        hdr->organizationSubType[2] = FLASH_PTP_RESPONSE_SUB_TYPE_2;
        hdr->flags = flags;
        len -= sizeof(*hdr);
        pos += sizeof(*hdr);

        if (len < sizeof(*error))
            return;
        error = (uint16_t*)&ptr[pos];
        *error = 0;
        len -= sizeof(*error);
        pos += sizeof(*error);

        if (len < sizeof(*reqIngressTimestamp))
            return;
        reqIngressTimestamp = (PTP2Timestamp*)&ptr[pos];
        *reqIngressTimestamp = PTP2Timestamp();
        len -= sizeof(*reqIngressTimestamp);
        pos += sizeof(*reqIngressTimestamp);

        if (len < sizeof(*reqCorrectionField))
            return;
        reqCorrectionField = (PTP2TimeInterval*)&ptr[pos];
        *reqCorrectionField = PTP2TimeInterval();
        len -= sizeof(*reqCorrectionField);
        pos += sizeof(*reqCorrectionField);

        if (len < sizeof(*utcOffset))
            return;
        utcOffset = (int16_t*)&ptr[pos];
        *utcOffset = 0;
        len -= sizeof(*utcOffset);
        pos += sizeof(*utcOffset);

        if (flags & FLASH_PTP_FLAG_SERVER_STATE_DS) {
            if (len < sizeof(*serverStateDS))
                return;
            serverStateDS = (FlashPTPServerStateDS*)&ptr[pos];
            *serverStateDS = FlashPTPServerStateDS();
            len -= sizeof(*serverStateDS);
            pos += sizeof(*serverStateDS);
        }

        hdr->tlvLength = pos;
        valid = true;
    }

    inline FlashPTPRespTLV() = default;
    inline uint16_t len() const { if (!valid) return 0; else return hdr->tlvLength; }
};

#pragma pack(pop)

using Log = cppLog::Log;
using Json = nlohmann::json;

}

#endif /* INCLUDE_FLASHPTP_COMMON_DEFINES_H_ */
