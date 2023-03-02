/*
 * @file inventory.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class Inventory represents the complete network inventory of a system
 * running flashPTP including physical network interfaces and assigned addresses.
 * It also provides all kinds of network related methods to get interface or
 * address information and to recv or send packets.
 *
 * The worker thread of the class periodically (10s) tries to detect changes like
 * interface status (up/down), new or removed interfaces or new or removed
 * IP addresses. Depending on the type of change, possibly open sockets might
 * be closed automatically.
 *
 * The class should not be instantiated and used directly, but the encapsulating
 * API functions provided by network.h/network.cpp should be used, instead.
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

#ifndef INCLUDE_FLASHPTP_NETWORK_INVENTORY_H_
#define INCLUDE_FLASHPTP_NETWORK_INVENTORY_H_

#include <flashptp/network/interface.h>
#include <flashptp/common/mode.h>

#define FLASH_PTP_INVENTORY_INTERVAL                    10

namespace flashptp {
namespace network {

class Inventory : public Thread {
public:
    ~Inventory();

    inline bool initialized() const { std::shared_lock sl(_mutex); return _initialized; }

    bool hasInterface(const std::string &intf);

    /*
     * Get the best timestamp level (user/socket/hardware) that is supported by
     * the network interface with the given name
     */
    PTPTimestampLevel getInterfaceTimestampLevel(const std::string &name);

    /*
     * Convert the MAC address of the network interface with the given name
     * to a PTP clock identity and store it in the provided struct
     */
    bool getInterfacePTPClockID(const std::string &intf, PTP2ClockID *clockID);

    // Get the PTP hardware clock name (i.e., /dev/ptp0) and id of the network interface with the given name
    bool getInterfacePHCInfo(const std::string &intf, std::string *name, clockid_t *id);

    // Get the id of the PTP hardware clock with the given name (i.e., /dev/ptp0)
    clockid_t getPHCClockIDByName(const std::string &name);

    /*
     * Check, if the given address is assigned to any of the systems network interfaces.
     * If so, store the interface name in the provided std::string (if provided)
     */
    bool hasAddress(const Address &addr, std::string *intf = nullptr);

    /*
     * Check, if the interface with the given name has an address of the given address family.
     * If so, store the address in the provided Address object (if provided)
     */
    bool getFamilyAddress(const std::string &intf, int family, Address *addr);

    /*
     * Try to receive incoming PTP Messages on sockets with the provided socket specs for msTimeout milliseconds.
     * Received packets will be forwarded to the onMsgReceived method of the provided Mode implementation.
     * Provide a buffer of sufficient length to store a received packet in. Returns the number of received packets.
     */
    int recv(uint8_t *buf, int len, const std::vector<SocketSpecs> &specs, uint16_t msTimeout, Mode *mode);

    /*
     * Try to send the provided PTP Message to the provided destination address (and destination UDP port)
     * via the source interface (and source UDP port). Try to get the tx timestamp if timestampLevel and
     * timestamp are provided. Set timestampLevel to the desired timestamp level prior to calling this function.
     */
    bool send(PTP2Message *msg, int len, const std::string &srcInterface, uint16_t srcPort, const Address &dstAddr,
            uint16_t dstPort, PTPTimestampLevel *timestampLevel = nullptr, struct timespec *timestamp = nullptr);

    void print();

protected:
    inline bool alwaysEnabled() const { return true; }
    inline const char *threadName() const { return "Network Inventory"; }
    /*
     * Periodically (10s) try to detect changes like interface status (up/down),
     * new or removed interfaces or new or removed IP addresses.
     */
    void threadFunc();

private:
    // The below function needs to be called with locked _mutex
    inline Interface *findInterface(const std::string &name)
    {
        for (auto intf: _interfaces) {
            if (intf->name() == name)
                return intf;
        }
        return nullptr;
    }

    mutable std::shared_mutex _mutex;
    bool _initialized{ false };
    std::vector<Interface*> _interfaces;
};

}
}

#endif /* INCLUDE_FLASHPTP_NETWORK_INVENTORY_H_ */
