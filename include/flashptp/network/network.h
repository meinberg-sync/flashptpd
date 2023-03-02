/*
 * @file network.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * This header provides all public network API functions to be used within
 * flashPTP. The init function instantiates a static object of class
 * network::Inventory, which is being used by all other API functions.
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

#ifndef INCLUDE_FLASHPTP_NETWORK_NETWORK_H_
#define INCLUDE_FLASHPTP_NETWORK_NETWORK_H_

#include <flashptp/network/inventory.h>

namespace flashptp {
namespace network {

void init();
bool initialized();
void print();
void exit();

bool hasInterface(const std::string &intf);

// @see Inventory::getInterfaceTimestampLevel
PTPTimestampLevel getInterfaceTimestampLevel(const std::string &intf);
// @see Inventory::getInterfacePTPClockID
bool getInterfacePTPClockID(const std::string &intf, PTP2ClockID *clockID);
// @see Inventory::getInterfacePHCInfo
bool getInterfacePHCInfo(const std::string &intf, std::string *name, clockid_t *id);

// @see Inventory::getPHCClockIDByName
clockid_t getPHCClockIDByName(const std::string &name);

// @see Inventory::hasAddress
bool hasAddress(const Address &addr, std::string *intf = nullptr);

// @see Inventory::getFamilyAddress
bool getFamilyAddress(const std::string &intf, int family, Address *addr = nullptr);

// @see Inventory::recv
int recv(uint8_t *buf, int len, const std::vector<SocketSpecs> &specs, uint16_t msTimeout, Mode *mode);
// @see Inventory::send
bool send(PTP2Message *msg, int len, const std::string &srcInterface, uint16_t srcPort, const Address &dstAddr,
        uint16_t dstPort, PTPTimestampLevel *timestampLevel = nullptr, struct timespec *timestamp = nullptr);

}
}

#endif /* INCLUDE_FLASHPTP_NETWORK_NETWORK_H_ */
