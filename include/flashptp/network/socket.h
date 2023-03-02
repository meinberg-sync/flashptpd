/*
 * @file socket.h
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class Socket can be used to open and use a POSIX socket on a specific
 * physical network interface and for a specific address family (LL2/IPv4/IPv6),
 * timestamp level (usr/so/hw) and UDP source port (if IPv4/IPv6).
 *
 * The struct SocketSpecs can be used to define a number of socket specifications,
 * (i.e., interface, family, timestamp level, port) to listen on (@see network::recv).
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

#ifndef INCLUDE_FLASHPTP_NETWORK_SOCKET_H_
#define INCLUDE_FLASHPTP_NETWORK_SOCKET_H_

#include <flashptp/network/address.h>

namespace flashptp {
namespace network {

class Interface;

struct SocketSpecs
{
    std::string interface;
    int family{ AF_UNSPEC };
    uint16_t srcPort{ 0 };
    PTPTimestampLevel timestampLevel{ PTPTimestampLevel::invalid };
    struct sockaddr_storage familySockaddr{ 0 };

    SocketSpecs() = default;

    inline SocketSpecs(const std::string &intf, int fam, uint16_t port, PTPTimestampLevel tslvl)
    {
        interface = intf;
        family = fam;
        srcPort = port;
        timestampLevel = tslvl;
    }
};

class Socket
{
public:
    /*
     * Create a new socket (file descriptor) of the given address family (AF_PACKET, AF_INET or AF_INET6)
     * bound to the provided network interface (and UDP source port if using AF_INET or AF_INET6) and
     * providing user level, socket (kernel level) or hardware timestamps.
     */
    Socket(const Interface *interface, int family, PTPTimestampLevel timestampLevel, uint16_t srcPort = 0);
    inline ~Socket() { if (_fd >= 0) close(_fd); }

    inline int family() const { return _family; }
    inline PTPTimestampLevel timestampLevel() const { return _timestampLevel; }
    inline uint16_t srcPort() const { return _srcPort; }

    inline bool valid() const { return _fd >= 0; }
    inline int fd() const { return _fd; }

    bool matches(int family, PTPTimestampLevel timestampLevel, uint16_t srcPort) const;
    bool matches(const SocketSpecs &specs) const;

    bool send(PTP2Message *msg, int len, const Address &dstAddr, uint16_t dstPort);
    PTPTimestampLevel transmitTimestamp(PTP2Message *msg, int len, PTPTimestampLevel desiredLevel,
            struct timespec *timestamp);

private:
    const Interface *_srcInterface;
    int _family{ AF_UNSPEC };
    PTPTimestampLevel _timestampLevel{ PTPTimestampLevel::invalid };
    uint16_t _srcPort{ 0 };

    int _fd{ -1 };

    char _cmsgbuf[1024];
    char _ctrlbuf[sizeof(struct cmsghdr) + 2048];

    struct sockaddr_storage _dstAddress;
    socklen_t _dstAddressLen{ 0 };
};

}
}

#endif /* INCLUDE_FLASHPTP_NETWORK_SOCKET_H_ */
