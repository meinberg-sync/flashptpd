/*
 * @file interface.cpp
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

#include <flashptp/network/interface.h>

namespace flashptp {
namespace network {

Interface::Interface(const char *name, int index, bool up, const struct sockaddr *macAddr)
{
    struct ifreq ifr;
    int fd;

    _name = name;
    setProperties(index, up, macAddr);

    memset(&ifr, 0, sizeof(ifr));
    _timestampInfo.cmd = ETHTOOL_GET_TS_INFO;
    strncpy(ifr.ifr_name, _name.c_str(), IFNAMSIZ - 1);
    ifr.ifr_data = (char*)&_timestampInfo;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return;

    if (ioctl(fd, SIOCETHTOOL, &ifr) < 0) {
        cppLog::warningf("Could not get timestamp info for %s: %s (%d)", _name.c_str(),
                strerror(errno), errno);
        return;
    }
    close(fd);

    if (_timestampInfo.phc_index < 0)
        return;

    _phc.init(std::string(FLASH_PTP_LINUX_PHC_IF) + std::to_string(_timestampInfo.phc_index));
}

void Interface::setProperties(int index, bool up, const struct sockaddr *macAddr)
{
    if (_index == index && _up == up && _macAddr.equals((const struct sockaddr_storage*)macAddr))
        return;

    clearSocks();
    _index = index;
    _up = up;
    _macAddr = Address((const struct sockaddr_storage*)macAddr);
}

void Interface::ptp2ClockID(struct PTP2ClockID *id)
{
    struct sockaddr_storage saddr;
    struct sockaddr_ll* sll = (struct sockaddr_ll*)&saddr;
    _macAddr.saddr(&saddr);
    id->b[0] = sll->sll_addr[0];
    id->b[1] = sll->sll_addr[1];
    id->b[2] = sll->sll_addr[2];
    id->b[3] = 0xff;
    id->b[4] = 0xfe;
    id->b[5] = sll->sll_addr[3];
    id->b[6] = sll->sll_addr[4];
    id->b[7] = sll->sll_addr[5];
}

void Interface::eraseIPAddr(unsigned index)
{
    if (index >= _ipAddrs.size())
        return;

    int family = _ipAddrs[index].family();
    _ipAddrs.erase(_ipAddrs.begin() + index);

    if (!getFamilyAddr(family)) {
        for (unsigned i = 0; i < _socks.size(); ++i) {
            if (_socks[i]->family() != family)
                continue;

            delete _socks[i];
            _socks.erase(_socks.begin() + i);
            --i;
        }
    }
}

const Address *Interface::getFamilyAddr(int family) const
{
    if (family == AF_PACKET)
        return &_macAddr;
    else if (family == AF_INET || family == AF_INET6) {
        for (unsigned i = 0; i < _ipAddrs.size(); ++i) {
            if (_ipAddrs[i].family() == family)
                return &_ipAddrs[i];
        }
    }
    return nullptr;
}

PTPTimestampLevel Interface::timestampLevel() const
{
    if (_phc.valid() &&
       (_timestampInfo.tx_types & (1UL << HWTSTAMP_TX_ON)) &&
       (_timestampInfo.so_timestamping & SOF_TIMESTAMPING_TX_HARDWARE) &&
       (_timestampInfo.so_timestamping & SOF_TIMESTAMPING_RX_HARDWARE) &&
       (_timestampInfo.so_timestamping & SOF_TIMESTAMPING_RAW_HARDWARE))
        return PTPTimestampLevel::hardware;
    else if ((_timestampInfo.so_timestamping & SOF_TIMESTAMPING_TX_SOFTWARE) &&
             (_timestampInfo.so_timestamping & SOF_TIMESTAMPING_RX_SOFTWARE) &&
             (_timestampInfo.so_timestamping & SOF_TIMESTAMPING_SOFTWARE))
        return PTPTimestampLevel::socket;
    else
        return PTPTimestampLevel::user;
}

void Interface::clearSocks()
{
    while (_socks.size()) {
        delete _socks.back();
        _socks.pop_back();
    }
}

Socket *Interface::sock(int family, PTPTimestampLevel timestampLevel, uint16_t srcPort)
{
    for (Socket *s: _socks) {
        if (s->matches(family, timestampLevel, srcPort))
            return s;
    }

    if (!getFamilyAddr(family))
        return nullptr;

    Socket *s = new Socket(this, family, timestampLevel, srcPort);
    if (!s->valid()) {
        delete s;
        s = nullptr;
    }

    _socks.push_back(s);
    return s;
}

}
}
