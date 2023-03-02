/*
 * @file interface.h
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class Interface represents a physical network interface within flashPTP.
 * An interface can have exactly one MAC address and any number of IP addresses.
 * Apart from that, an interface provides information about the possibly provided
 * physical hardware clock (PHC) and stores currently open file descriptors (sockets).
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

#ifndef INCLUDE_FLASHPTP_NETWORK_INTERFACE_H_
#define INCLUDE_FLASHPTP_NETWORK_INTERFACE_H_

#include <flashptp/network/socket.h>
#include <flashptp/network/phc.h>

namespace flashptp {
namespace network {

class Interface {
public:
    Interface(const char *name, int index, bool up, const struct sockaddr *macAddr);
    inline ~Interface() { clearSocks(); }

    // Update the provided interface information, close all open sockets if anything changes
    void setProperties(int index, bool up, const struct sockaddr *macAddr);

    inline const std::string &name() const { return _name; }
    inline int index() const { return _index; }
    inline bool up() const { return _up; }

    inline const Address &macAddr() const { return _macAddr; }
    /*
     * Convert the MAC address of the interface to a PTP clock identity by inserting 0xFFFE in the middle.
     * MAC address 0xEC4670123456 becomes PTP clock identity 0xEC4670FFFE123456.
     */
    void ptp2ClockID(struct PTP2ClockID *id);

    inline void addIPAddr(const struct sockaddr *ipAddr, const struct sockaddr *ipPrefix = nullptr)
    { _ipAddrs.emplace_back((const struct sockaddr_storage*)ipAddr, (const struct sockaddr_storage*)ipPrefix); }
    inline void addIPAddr(const Address &addr) { _ipAddrs.push_back(addr); }
    inline unsigned countIPAddrs() const { return _ipAddrs.size(); }
    void eraseIPAddr(unsigned index);
    inline Address *ipAddr(unsigned index)
    { if (index < _ipAddrs.size()) return &_ipAddrs[index]; return nullptr; }

    // Get (first) MAC or IP address of the provided address family (AF_PACKET, AF_INET, AF_INET6).
    const Address *getFamilyAddr(int family) const;

    // Get the best timestamp level (hardware/socket/user) that is supported by the interface
    PTPTimestampLevel timestampLevel() const;
    inline const struct ethtool_ts_info &timestampInfo() const { return _timestampInfo; }

    inline const PHC &phc() const { return _phc; }

    void clearSocks();
    inline unsigned countSocks() const { return _socks.size(); }
    /*
     * Try to find an open socket with the given address family, timestamp level and UDP source port.
     * If no socket is found and the interface does have an address of the given address family,
     * open a new socket.
     */
    Socket *sock(int family, PTPTimestampLevel timestampLevel, uint16_t srcPort = 0);
    inline Socket *sock(unsigned index)
    { if (index < _socks.size()) return _socks[index]; return nullptr; }

private:
    std::string _name;
    int _index{ 0 };
    bool _up{ false };

    Address _macAddr;
    std::vector<Address> _ipAddrs;

    struct ethtool_ts_info _timestampInfo{ 0 };
    PHC _phc;

    std::vector<Socket*> _socks;
};

}
}

#endif /* INCLUDE_FLASHPTP_NETWORK_INTERFACE_H_ */
