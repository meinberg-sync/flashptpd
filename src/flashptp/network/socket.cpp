/*
 * @file socket.cpp
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
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

#include <flashptp/network/socket.h>
#include <flashptp/network/interface.h>

namespace flashptp {
namespace network {

Socket::Socket(const Interface *interface, int family, PTPTimestampLevel timestampLevel, uint16_t srcPort)
{
    _srcInterface = interface;
    if (!_srcInterface)
        return;

    _family = family;
    _timestampLevel = timestampLevel;
    _srcPort = srcPort;
    switch (family) {
    case AF_PACKET: {
        _fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_1588));
        _srcPort = 0;
        break;
    }
    case AF_INET: _fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); break;
    case AF_INET6: _fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP); break;
    default: break;
    }

    if (_fd < 0) {
        cppLog::errorf("%s socket could not be opened: %s (%d)", Address::familyToStr(family),
                strerror(errno), errno);
        return;
    }

    struct sockaddr_storage bindaddr;
    socklen_t bindaddrlen = 0;
    struct ifreq ifr;
    int opt;

    if (family == AF_INET) {
        opt = 1;
        if (setsockopt(_fd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt)) != 0) {
            cppLog::errorf("%s socket option IP_PKTINFO (%d) could not be set: %s (%d)",
                    Address::familyToStr(family), opt, strerror(errno), errno);
            goto err_out;
        }
    }
    else if (family == AF_INET6) {
        opt = 1;
        if (setsockopt(_fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) != 0) {
            cppLog::errorf("%s socket option IPV6_V6ONLY (%d) could not be set: %s (%d)",
                    Address::familyToStr(family), opt, strerror(errno), errno);
            goto err_out;
        }

        opt = 1;
        if (setsockopt(_fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &opt, sizeof(opt)) != 0) {
            cppLog::errorf("%s socket option IPV6_RECVPKTINFO (%d) could not be set: %s (%d)",
                    Address::familyToStr(family), opt, strerror(errno), errno);
            goto err_out;
        }

    }

    opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        cppLog::errorf("%s socket option SO_REUSEADDR (%d) could not be set: %s (%d)",
                Address::familyToStr(family), opt, strerror(errno), errno);
        goto err_out;
    }

    memset(&bindaddr, 0, sizeof(bindaddr));
    bindaddr.ss_family = family;

    switch (family) {
    case AF_PACKET: {
        struct sockaddr_ll *sll = (struct sockaddr_ll*)&bindaddr;
        sll->sll_protocol = htons(ETH_P_1588);
        sll->sll_ifindex = _srcInterface->index();
        bindaddrlen = sizeof(*sll);
        break;
    }
    case AF_INET: {
        struct sockaddr_in *sin = (struct sockaddr_in*)&bindaddr;
        sin->sin_addr.s_addr = htonl(INADDR_ANY);
        sin->sin_port = htons(_srcPort);
        bindaddrlen = sizeof(*sin);
        break;
    }
    case AF_INET6: {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&bindaddr;
        sin6->sin6_addr = in6addr_any;
        sin6->sin6_port = htons(_srcPort);
        sin6->sin6_scope_id = _srcInterface->index();
        bindaddrlen = sizeof(*sin6);
        break;
    }
    default: break;
    }

    if (bindaddrlen > 0 &&
        bind(_fd, (struct sockaddr*)&bindaddr, bindaddrlen) == -1) {
        cppLog::errorf("%s socket could not be bound to port %u: %s (%d)", Address::familyToStr(family),
                _srcPort, strerror(errno), errno);
        goto err_out;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, _srcInterface->name().c_str(), IFNAMSIZ);

    if (setsockopt(_fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) != 0) {
        cppLog::errorf("%s socket could not be bound to interface %s: %s (%d)", Address::familyToStr(family),
                _srcInterface->name().c_str(), strerror(errno), errno);
        goto err_out;
    }

    if (_timestampLevel > _srcInterface->timestampLevel())
        _timestampLevel = _srcInterface->timestampLevel();

    if (_timestampLevel == PTPTimestampLevel::hardware) {
        struct hwtstamp_config hwTimestamp;
        const auto &timestampInfo = _srcInterface->timestampInfo();

        memset(&hwTimestamp, 0, sizeof(hwTimestamp));
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, _srcInterface->name().c_str(), IFNAMSIZ);
        ifr.ifr_data = (char*)&hwTimestamp;

        hwTimestamp.tx_type = HWTSTAMP_TX_ON;

        if (family == AF_PACKET) {
            if (timestampInfo.rx_filters & (1UL << HWTSTAMP_FILTER_PTP_V2_L2_SYNC))
                hwTimestamp.rx_filter = HWTSTAMP_FILTER_PTP_V2_L2_SYNC;
            else if (timestampInfo.rx_filters & (1UL << HWTSTAMP_FILTER_PTP_V2_L2_EVENT))
                hwTimestamp.rx_filter = HWTSTAMP_FILTER_PTP_V2_L2_EVENT;
        }
        else if (srcPort == FLASH_PTP_UDP_EVENT_PORT) {
            if (timestampInfo.rx_filters & (1UL << HWTSTAMP_FILTER_PTP_V2_L4_SYNC))
                hwTimestamp.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_SYNC;
            else if (timestampInfo.rx_filters & (1UL << HWTSTAMP_FILTER_PTP_V2_L4_EVENT))
                hwTimestamp.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_EVENT;
        }
        else
            hwTimestamp.rx_filter = HWTSTAMP_FILTER_ALL;

        if (hwTimestamp.rx_filter == 0) {
            if (timestampInfo.rx_filters & (1UL << HWTSTAMP_FILTER_PTP_V2_SYNC))
                hwTimestamp.rx_filter = HWTSTAMP_FILTER_PTP_V2_SYNC;
            else if (timestampInfo.rx_filters & (1UL << HWTSTAMP_FILTER_PTP_V2_EVENT))
                hwTimestamp.rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
            else
                hwTimestamp.rx_filter = HWTSTAMP_FILTER_ALL;
        }

        if (ioctl(_fd, SIOCSHWTSTAMP, &ifr) < 0) {
            cppLog::errorf("%s socket (%s) hardware timestamp config could not be applied: %s (%d)",
                    Address::familyToStr(family), _srcInterface->name().c_str(), strerror(errno), errno);
            goto err_out;
        }

        opt = (SOF_TIMESTAMPING_RAW_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_TX_HARDWARE);
        opt |= (SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE);
    }
    else if (_timestampLevel == PTPTimestampLevel::socket)
        opt = (SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE);
    else
        opt = 0;

    if (opt > 0 &&
        setsockopt(_fd, SOL_SOCKET, SO_TIMESTAMPING, &opt, sizeof(opt)) != 0) {
        cppLog::errorf("%s socket (%s) option SO_TIMESTAMPING (%08x) could not be set: %s (%d)",
                Address::familyToStr(family), _srcInterface->name().c_str(), opt, strerror(errno), errno);
        goto err_out;

        opt = 1;
        if (setsockopt(_fd, SOL_SOCKET, SO_SELECT_ERR_QUEUE, &opt, sizeof(opt)) != 0) {
            cppLog::errorf("%s socket (%s) option SO_SELECT_ERR_QUEUE (%d) could not be set: %s (%d)",
                    Address::familyToStr(family), _srcInterface->name().c_str(), opt, strerror(errno), errno);
            goto err_out;
        }
    }

    cppLog::debugf("%s socket (%s%s%s, %s timestamping) opened, successfully", Address::familyToStr(family),
            _srcInterface->name().c_str(), family == AF_PACKET ? "" : ", UDP Port ",
            family == AF_PACKET ? "" : std::to_string(_srcPort).c_str(), ptpTimestampLevelToStr(_timestampLevel));

    return;

err_out:
    if (_fd >= 0)
        close(_fd);
}

bool Socket::matches(int family, PTPTimestampLevel timestampLevel, uint16_t srcPort) const
{
    if (family != _family || timestampLevel > _timestampLevel)
        return false;

    if (family == AF_INET || family == AF_INET6)
        return srcPort == _srcPort;
    else
        return true;
}

bool Socket::matches(const SocketSpecs &specs) const
{
    if (specs.interface != _srcInterface->name())
        return false;
    return matches(specs.family, specs.timestampLevel, specs.srcPort);
}

bool Socket::send(PTP2Message *msg, int len, const Address &dstAddr, uint16_t dstPort)
{
    dstAddr.saddr(&_dstAddress);
    switch (_dstAddress.ss_family) {
    case AF_PACKET: {
        ((struct sockaddr_ll*)&_dstAddress)->sll_protocol = htons(ETH_P_1588);
        ((struct sockaddr_ll*)&_dstAddress)->sll_ifindex = _srcInterface->index();
        _dstAddressLen = sizeof(struct sockaddr_ll);
        break;
    }
    case AF_INET: {
        ((struct sockaddr_in*)&_dstAddress)->sin_port = htons(dstPort);
        _dstAddressLen = sizeof(struct sockaddr_in);
        break;
    }
    case AF_INET6: {
        ((struct sockaddr_in6*)&_dstAddress)->sin6_port = htons(dstPort);
        ((struct sockaddr_in6*)&_dstAddress)->sin6_scope_id = _srcInterface->index();
        _dstAddressLen = sizeof(struct sockaddr_in6);
        break;
    }
    default: return false;
    }

    if (sendto(_fd, msg, len, 0, (const struct sockaddr*)&_dstAddress, _dstAddressLen) != len) {
        cppLog::errorf("Could not sent %s %s Message to %s: %s (%d)",
                ptpMessageTypeToStr((PTPMessageType)msg->msgType),
                len > sizeof(*msg) ? (msg->logMsgPeriod != 0x7f ? "Request" : "Response") : "Message",
                dstAddr.str().c_str(), strerror(errno), errno);
        return false;
    }

    return true;
}

PTPTimestampLevel Socket::transmitTimestamp(PTP2Message *msg, int len, PTPTimestampLevel desiredLevel,
        struct timespec *timestamp)
{
    struct iovec entry = { _cmsgbuf, sizeof(_cmsgbuf) };
    PTPTimestampLevel timestampLevel;
    struct timespec *spec = nullptr;
    struct sockaddr_in sin;
    struct cmsghdr *cmhdr;
    struct msghdr mhdr;
    struct pollfd pfd;
    int n;

    timestampLevel = PTPTimestampLevel::invalid;
    if (desiredLevel < PTPTimestampLevel::socket)
        goto get_usr_timestamp;

    memset(&mhdr, 0, sizeof(mhdr));
    mhdr.msg_iov = &entry;
    mhdr.msg_iovlen = 1;
    mhdr.msg_name = (caddr_t)&sin;
    mhdr.msg_namelen = sizeof(sin);
    mhdr.msg_control = _ctrlbuf;
    mhdr.msg_controllen = sizeof(_ctrlbuf);

get_so_timestamp:
    pfd.fd = _fd;
    pfd.events = 0;
    pfd.revents = 0;
    n = poll(&pfd, 1, 100);
    if (n > 0) {
        if (!(pfd.revents & POLLERR))
            goto get_so_timestamp;

        n = recvmsg(_fd, &mhdr, MSG_ERRQUEUE);
        if (n < len || memcmp(msg, &_cmsgbuf[n - len], len) != 0)
            goto get_so_timestamp;

        for (cmhdr = CMSG_FIRSTHDR(&mhdr); cmhdr; cmhdr = CMSG_NXTHDR(&mhdr, cmhdr)) {
            if (cmhdr->cmsg_level != SOL_SOCKET ||
                cmhdr->cmsg_type != SO_TIMESTAMPING)
                continue;

            spec = (struct timespec*)CMSG_DATA(cmhdr);
            if (cmhdr->cmsg_len >= sizeof(*spec) &&
                spec[0].tv_sec > 0) {
                timestampLevel = PTPTimestampLevel::socket;
                *timestamp = spec[0];
            }

            if (cmhdr->cmsg_len >= (3 * sizeof(*spec)) &&
                spec[2].tv_sec > 0 &&
                _timestampLevel == PTPTimestampLevel::hardware) {
                timestampLevel = PTPTimestampLevel::hardware;
                *timestamp = spec[2];
            }
            break;
        }
    }

get_usr_timestamp:
    if (timestampLevel == PTPTimestampLevel::invalid &&
        clock_gettime(CLOCK_REALTIME, timestamp) == 0)
        timestampLevel = PTPTimestampLevel::user;

    return timestampLevel;
}

}
}
