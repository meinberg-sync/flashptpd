/*
 * @file inventory.cpp
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

#include <flashptp/network/inventory.h>

namespace flashptp {
namespace network {

Inventory::~Inventory()
{
    while (_interfaces.size()) {
        delete _interfaces.back();
        _interfaces.pop_back();
    }
}

bool Inventory::hasInterface(const std::string &intf)
{
    std::shared_lock sl(_mutex);
    return findInterface(intf) != nullptr;
}

PTPTimestampLevel Inventory::getInterfaceTimestampLevel(const std::string &intf)
{
    std::shared_lock sl(_mutex);
    Interface *interface = findInterface(intf);
    if (interface)
        return interface->timestampLevel();
    else
        return PTPTimestampLevel::invalid;
}

bool Inventory::getInterfacePTPClockID(const std::string &intf, PTP2ClockID *clockID)
{
    clockID->reset();

    std::shared_lock sl(_mutex);
    Interface *interface = findInterface(intf);
    if (!interface)
        return false;

    interface->ptp2ClockID(clockID);
    return true;
}

bool Inventory::getInterfacePHCInfo(const std::string &intf, std::string *name, clockid_t *id)
{
    if (name)
        name->clear();

    if (id)
        *id = -1;

    std::shared_lock sl(_mutex);
    Interface *interface = findInterface(intf);
    if (!interface || !interface->phc().valid())
        return false;

    if (name)
        *name = interface->phc().name();

    if (id)
        *id = interface->phc().id();

    return true;
}

clockid_t Inventory::getPHCClockIDByName(const std::string &name)
{
    std::shared_lock sl(_mutex);
    Interface *interface;
    for (unsigned i = 0; i < _interfaces.size(); ++i) {
        interface = _interfaces[i];
        if (interface->phc().valid() && interface->phc().name() == name)
            return interface->phc().id();
    }

    return -1;
}

bool Inventory::hasAddress(const Address &addr, std::string *intf)
{
    Interface *interface;
    unsigned i, j;

    if (intf)
        intf->clear();

    std::shared_lock sl(_mutex);
    for (i = 0; i < _interfaces.size(); ++i) {
        interface = _interfaces[i];
        if (addr.family() == AF_PACKET) {
            if (interface->macAddr().shortStr() == addr.shortStr()) {
                *intf = interface->name();
                return true;
            }
            else
                continue;
        }

        for (j = 0; j < interface->countIPAddrs(); ++j) {
            if (interface->ipAddr(j)->shortStr() == addr.shortStr()) {
                *intf = interface->name();
                return true;
            }
        }
    }

    return false;
}

bool Inventory::getFamilyAddress(const std::string &intf, int family, Address *addr)
{
    if (addr)
        *addr = Address();

    std::shared_lock sl(_mutex);
    Interface *interface = findInterface(intf);
    if (!interface)
        return false;

    if (family == AF_PACKET) {
        if (addr)
            *addr = interface->macAddr();
        return true;
    }

    Address *address;
    for (unsigned i = 0; i < interface->countIPAddrs(); ++i) {
        address = interface->ipAddr(i);
        if (address->family() == family) {
            if (addr)
                *addr = *address;
            return true;
        }
    }

    return false;
}

int Inventory::recv(uint8_t *buf, int len, const std::vector<SocketSpecs> &specs, uint16_t msTimeout, Mode *mode)
{
    if (!buf || len < sizeof(PTP2Message) || specs.empty() || msTimeout == 0 || mode == nullptr)
        return -1;

    struct timeval tv{ msTimeout / 1000, (msTimeout % 1000 * 1000) };
    int n, totalCnt, readyFds, maxFd = -1;
    unsigned i;

    std::unordered_map<int, const SocketSpecs> fds;
    Interface *interface;
    fd_set set, backup;
    SocketSpecs spec;
    Socket *s;

    _mutex.lock();

    FD_ZERO(&backup);

    /*
     * Loop through the provided socket specifications and store the fd and specs in the fds map
     * and in the fd_set backup to be copied to fd_set set later and to be used for select call(s).
     */
    for (const auto &si: specs) {
        interface = findInterface(si.interface);
        if (!interface || !interface->up())
            continue;

        spec = si;
        if (spec.timestampLevel > interface->timestampLevel())
            spec.timestampLevel = interface->timestampLevel();

        s = interface->sock(spec.family, spec.timestampLevel, spec.srcPort);
        if (!s)
            continue;

        if (fds.find(s->fd()) != fds.end())
            continue;

        interface->getFamilyAddr(spec.family)->saddr(&spec.familySockaddr);
        fds.emplace(s->fd(), std::move(spec));
        FD_SET(s->fd(), &backup);
        if (s->fd() > maxFd)
            maxFd = s->fd();
    }

    _mutex.unlock();

    if (fds.size() == 0)
        return -1;

    struct sockaddr_storage srcSockaddr, dstSockaddr;
    char ctrl[sizeof(struct cmsghdr) + 2048];
    struct iovec entry { buf, (size_t)len };
    struct timespec timestamp, rtts, *tspec;
    PTPTimestampLevel timestampLevel;
    struct msghdr mhdr;
    PTP2Message *msg;
    Address dstAddr;

    totalCnt = 0;

select_set:
    // Restore fd_set from backed up set for next select call
    set = backup;
    readyFds = select(maxFd + 1, &set, NULL, NULL, &tv);
    if (readyFds <= 0) {
        if (readyFds < 0)
            cppLog::errorf("Sockets could not be monitored (select() call failed): %s (%d)",
                    strerror(errno), errno);
        return totalCnt;
    }

    clock_gettime(CLOCK_REALTIME, &rtts);

    for (const auto &fd: fds) {
        if (!FD_ISSET(fd.first, &set))
            continue;

        timestampLevel = PTPTimestampLevel::user;
        timestamp = rtts;

        memset(&mhdr, 0, sizeof(mhdr));
        mhdr.msg_iov = &entry;
        mhdr.msg_iovlen = 1;
        mhdr.msg_name = &srcSockaddr;
        mhdr.msg_namelen = sizeof(srcSockaddr);
        mhdr.msg_control = ctrl;
        mhdr.msg_controllen = sizeof(ctrl);

        n = recvmsg(fd.first, &mhdr, MSG_DONTWAIT);
        if (n < 0) {
            /*
             * error in recvmsg: continue with next fd, but decrement the number of
             * ready descriptors in order to prevent an infinite select loop
             */
            --readyFds;
            continue;
        }

        if (n < (int)sizeof(PTP2Message))
            continue;

        msg = (PTP2Message*)buf;
        if (msg->version != (uint8_t)FLASH_PTP_FIXED_VERSION ||
           (((uint16_t)msg->sdoIDMajor << 8) | (uint16_t)msg->sdoIDMinor) != FLASH_PTP_FIXED_SDO_ID ||
            msg->domain != FLASH_PTP_FIXED_DOMAIN_NUMBER ||
            msg->flags.unicast == 0)
            continue;

        dstSockaddr.ss_family = AF_UNSPEC;

        for (struct cmsghdr *cmhdr = CMSG_FIRSTHDR(&mhdr); cmhdr; cmhdr = CMSG_NXTHDR(&mhdr, cmhdr)) {
            if (cmhdr->cmsg_level == SOL_IP && cmhdr->cmsg_type == IP_PKTINFO) {
                dstSockaddr.ss_family = AF_INET;
                ((struct sockaddr_in*)&dstSockaddr)->sin_port = htons(fd.second.srcPort);
                ((struct sockaddr_in*)&dstSockaddr)->sin_addr =
                        ((struct in_pktinfo*)CMSG_DATA(cmhdr))->ipi_addr;
            }
            else if (cmhdr->cmsg_level == SOL_IPV6 && cmhdr->cmsg_type == IPV6_PKTINFO) {
                dstSockaddr.ss_family = AF_INET6;
                ((struct sockaddr_in6*)&dstSockaddr)->sin6_port = htons(fd.second.srcPort);
                ((struct sockaddr_in6*)&dstSockaddr)->sin6_addr =
                        ((struct in6_pktinfo*)CMSG_DATA(cmhdr))->ipi6_addr;
            }
            else if (cmhdr->cmsg_level == SOL_SOCKET && cmhdr->cmsg_type == SO_TIMESTAMPING) {
                tspec = (struct timespec*)CMSG_DATA(cmhdr);
                if (cmhdr->cmsg_len >= (3 * sizeof(struct timespec)) &&
                    tspec[2].tv_sec > 0 &&
                    fd.second.timestampLevel == PTPTimestampLevel::hardware) {
                    timestampLevel = PTPTimestampLevel::hardware;
                    timestamp = tspec[2];
                }
                else if (cmhdr->cmsg_len >= sizeof(struct timespec) &&
                         tspec[0].tv_sec > 0 &&
                         fd.second.timestampLevel >= PTPTimestampLevel::socket) {
                    timestampLevel = PTPTimestampLevel::socket;
                    timestamp = tspec[0];
                }
            }
        }

        if (dstSockaddr.ss_family == AF_UNSPEC) {
            dstSockaddr = fd.second.familySockaddr;
            if (dstSockaddr.ss_family == AF_INET)
                ((struct sockaddr_in*)&dstSockaddr)->sin_port = htons(fd.second.srcPort);
            else if (dstSockaddr.ss_family == AF_INET6)
                ((struct sockaddr_in6*)&dstSockaddr)->sin6_port = htons(fd.second.srcPort);
        }

        mode->onMsgReceived((PTP2Message*)buf, n, &srcSockaddr, &dstSockaddr, timestampLevel, &timestamp);
        ++totalCnt;
    }

    /*
     * if the number of ready descriptors is still bigger than zero,
     * meaning that none or at least not all of the ready descriptors had errors in recvmsg,
     * continue with select until the timeout expires or next descriptor(s) is/are ready
     */
    if (readyFds > 0 && (tv.tv_sec > 0 || tv.tv_usec > 0))
        goto select_set;

    return totalCnt;
}

bool Inventory::send(PTP2Message *msg, int len, const std::string &srcInterface, uint16_t srcPort,
        const Address &dstAddr, uint16_t dstPort, PTPTimestampLevel *timestampLevel, struct timespec *timestamp)
{
    if (!msg || len < sizeof(*msg) || srcInterface.empty() || !dstAddr.valid())
        return false;

    if ((dstAddr.family() == AF_INET || dstAddr.family() == AF_INET6) &&
        (srcPort == 0 || dstPort == 0))
        return false;

    std::unique_lock ul(_mutex);

    Interface *interface = findInterface(srcInterface);
    if (!interface || !interface->up())
        return false;

    Socket *sock;
    if (timestampLevel && timestamp) {
        if (*timestampLevel > interface->timestampLevel())
            *timestampLevel = interface->timestampLevel();
        sock = interface->sock(dstAddr.family(), *timestampLevel, srcPort);
    }
    else
        sock = interface->sock(dstAddr.family(), PTPTimestampLevel::invalid, srcPort);

    if (!sock)
        return false;

    interface->ptp2ClockID(&msg->portIdentity.clockID);
    msg->portIdentity.portID = htons(1);

    if (timestampLevel && timestamp) {
        if (!msg->flags.twoStep)
            *timestampLevel = PTPTimestampLevel::user;

        if (*timestampLevel == PTPTimestampLevel::user) {
            clock_gettime(CLOCK_REALTIME, timestamp);
            msg->timestamp = *timestamp;
            msg->timestamp.reorder();
        }
        else if (*timestampLevel == PTPTimestampLevel::hardware)
            msg->flags.timescale = 1;
    }

    if (!sock->send(msg, len, dstAddr, dstPort))
        return false;

    if (timestampLevel && timestamp) {
        *timestampLevel = sock->transmitTimestamp(msg, len, *timestampLevel, timestamp);
        cppLog::tracef("Sent %s %s (seq id %u, %s timestamp) to %s, successfully",
                ptpMessageTypeToStr((PTPMessageType)msg->msgType),
                len > sizeof(*msg) ? (msg->logMsgPeriod != 0x7f ? "Request" : "Response") : "Message",
                ntohs(msg->seqID), ptpTimestampLevelToShortStr(*timestampLevel),
                dstAddr.str().c_str());
    }
    else {
        cppLog::tracef("Sent %s %s (seq id %u) to %s, successfully",
                ptpMessageTypeToStr((PTPMessageType)msg->msgType),
                len > sizeof(*msg) ? (msg->logMsgPeriod != 0x7f ? "Request" : "Response") : "Message",
                ntohs(msg->seqID), dstAddr.str().c_str());
    }

    return true;
}

void Inventory::print()
{
    Interface *interface;
    Address *addr;
    unsigned i, j;

    std::shared_lock sl(_mutex);
    for (i = 0; i < _interfaces.size(); ++i) {
        interface = _interfaces[i];
        printf("%d: %s\n", interface->index(), interface->name().c_str());
        printf("\tether %s\n", interface->macAddr().str().c_str());

        for (j = 0; j < interface->countIPAddrs(); ++j) {
            addr = interface->ipAddr(j);
            if (addr->family() != AF_INET)
                continue;
            printf("\tinet %s\n", addr->str().c_str());
        }

        for (j = 0; j < interface->countIPAddrs(); ++j) {
            addr = interface->ipAddr(j);
            if (addr->family() != AF_INET6)
                continue;
            printf("\tinet6 %s\n", addr->str().c_str());
        }

        if (!interface->phc().valid()) {
            printf("\tphc none/unknown\n");
            continue;
        }

        printf("\tphc %s\n", interface->phc().name().c_str());
    }
}

void Inventory::threadFunc()
{
    struct ifaddrs *ifaddr = nullptr, *ifa;
    std::vector<Interface*>::iterator iit;
    std::vector<Address> ipAddrs;
    std::vector<int> indices;
    std::string::size_type pos;
    std::string name;
    Interface *intfp;
    Address *addrp;
    unsigned i, j;
    int s = 0;
    bool up;

    while (_running) {
        if (--s > 0)
            goto next_iter;

        s = FLASH_PTP_INVENTORY_INTERVAL;

        _mutex.lock();

        if (_initialized)
            cppLog::tracef("Running periodical Network Inventory update (%d seconds)", FLASH_PTP_INVENTORY_INTERVAL);
        else
            cppLog::tracef("Setting up Network Inventory (interfaces and addresses)");

        if (getifaddrs(&ifaddr) == -1) {
            cppLog::errorf("Could not get Network Inventory information: %s (%d)", strerror(errno), errno);
            goto next_iter_unlock;
        }

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr ||
                ifa->ifa_addr->sa_family != AF_PACKET)
                continue;

            indices.emplace_back(if_nametoindex(ifa->ifa_name));
            if (_initialized)
                intfp = findInterface(ifa->ifa_name);
            else
                intfp = nullptr;

            up = (ifa->ifa_flags & IFF_UP) && (ifa->ifa_flags & IFF_LOWER_UP);
            if (!intfp) {
                _interfaces.push_back(new Interface(ifa->ifa_name, indices.back(), up, ifa->ifa_addr));
                intfp = _interfaces.back();
                cppLog::infof("Network interface %s (%s) [%d] %s", intfp->name().c_str(),
                        intfp->macAddr().str().c_str(), intfp->index(),
                        _initialized ? "has been added" : "detected");
            }
            else {
                if (intfp->up() != up)
                    cppLog::infof("Link state of interface %s changed from %s to %s", intfp->name().c_str(),
                            intfp->up() ? "up" : "down", up ? "up" : "down");
                intfp->setProperties(indices.back(), up, ifa->ifa_addr);
            }
        }

        if (_initialized) {
            for (iit = _interfaces.begin(); iit != _interfaces.end();) {
                if (std::find(indices.begin(), indices.end(), (*iit)->index()) == indices.end()) {
                    cppLog::infof("Network interface %s (%s) [%d] has been removed", (*iit)->name().c_str(),
                            (*iit)->macAddr().str().c_str(), (*iit)->index());
                    delete *iit;
                    iit = _interfaces.erase(iit);
                }
                else
                    ++iit;
            }
        }

        for (iit = _interfaces.begin(); iit != _interfaces.end(); ++iit) {
            for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == nullptr ||
                    ifa->ifa_addr->sa_family == AF_PACKET)
                    continue;

                name = ifa->ifa_name;
                pos = name.find(':');
                if (pos != std::string::npos)
                    name.erase(pos);
                if (name != (*iit)->name())
                    continue;

                if (!_initialized)
                    (*iit)->addIPAddr(ifa->ifa_addr, ifa->ifa_netmask);
                else
                    ipAddrs.emplace_back((const struct sockaddr_storage*)ifa->ifa_addr,
                            (const struct sockaddr_storage*)ifa->ifa_netmask);
            }

            if (_initialized) {
                for (i = 0; i < ipAddrs.size(); ++i) {
                    for (j = 0; j < (*iit)->countIPAddrs(); ++j) {
                        if (ipAddrs[i].str() == (*iit)->ipAddr(j)->str())
                            break;
                    }

                    if (j == (*iit)->countIPAddrs()) {
                        (*iit)->addIPAddr(ipAddrs[i]);
                        cppLog::infof("Added address %s to interface %s", ipAddrs[i].str().c_str(),
                                (*iit)->name().c_str());
                    }
                }

                for (i = 0; i < (*iit)->countIPAddrs(); ++i) {
                    addrp = (*iit)->ipAddr(i);
                    for (j = 0; j < ipAddrs.size(); ++j) {
                        if (addrp->str() == ipAddrs[j].str())
                            break;
                    }

                    if (j == ipAddrs.size()) {
                        cppLog::infof("Removed address %s from interface %s", addrp->str().c_str(),
                                (*iit)->name().c_str());
                        (*iit)->eraseIPAddr(i--);
                    }
                }
            }

            ipAddrs.clear();
        }

        freeifaddrs(ifaddr);
        ifaddr = nullptr;
        _initialized = true;

next_iter_unlock:
        _mutex.unlock();
next_iter:
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

}
}
