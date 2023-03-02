/*
 * @file network.cpp
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

#include <flashptp/network/network.h>

namespace flashptp {
namespace network {

static Inventory *__inventory;

void init()
{
    exit();
    __inventory = new Inventory();
    __inventory->start();
}

bool initialized()
{
    if (__inventory)
        return __inventory->initialized();
    else
        return false;
}

void print()
{
    if (__inventory && __inventory->initialized())
        __inventory->print();
    else
        printf("Network Inventory could not be initialized!\n");
}

void exit()
{
    if (__inventory) {
        __inventory->stop();
        delete __inventory;
    }
    __inventory = nullptr;
}

bool hasInterface(const std::string &intf)
{
    if (__inventory)
        return __inventory->hasInterface(intf);
    else
        return false;
}

PTPTimestampLevel getInterfaceTimestampLevel(const std::string &intf)
{
    if (__inventory)
        return __inventory->getInterfaceTimestampLevel(intf);
    else
        return PTPTimestampLevel::invalid;
}

bool getInterfacePTPClockID(const std::string &intf, PTP2ClockID *clockID)
{
    if (__inventory)
        return __inventory->getInterfacePTPClockID(intf, clockID);
    else {
        if (clockID)
            clockID->reset();
        return false;
    }
}

bool getInterfacePHCInfo(const std::string &intf, std::string *name, clockid_t *id)
{
    if (__inventory)
        return __inventory->getInterfacePHCInfo(intf, name, id);
    else {
        if (name)
            name->clear();
        if (id)
            *id = -1;
        return false;
    }
}

clockid_t getPHCClockIDByName(const std::string &name)
{
    if (__inventory)
        return __inventory->getPHCClockIDByName(name);
    else
        return -1;
}

bool hasAddress(const Address &addr, std::string *intf)
{
    if (__inventory)
        return __inventory->hasAddress(addr, intf);
    else {
        if (intf)
            intf->clear();
        return false;
    }
}

bool getFamilyAddress(const std::string &intf, int family, Address *addr)
{
    if (__inventory)
        return __inventory->getFamilyAddress(intf, family, addr);
    else {
        if (addr)
            *addr = Address();
        return false;
    }
}

int recv(uint8_t *buf, int len, const std::vector<SocketSpecs> &specs, uint16_t msTimeout, Mode *mode)
{
    if (__inventory)
        return __inventory->recv(buf, len, specs, msTimeout, mode);
    else
        return -1;
}

bool send(PTP2Message *msg, int len, const std::string &srcInterface, uint16_t srcPort, const Address &dstAddr,
        uint16_t dstPort, PTPTimestampLevel *timestampLevel, struct timespec *timestamp)
{
    if (__inventory)
        return __inventory->send(msg, len, srcInterface, srcPort, dstAddr, dstPort, timestampLevel, timestamp);
    else
        return false;
}

}
}
