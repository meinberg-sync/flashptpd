/*
 * @file phc.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class PHC provides name (i.e., /dev/ptp0) and id of a physical hardware
 * clock (PHC) belonging to a physical network interface.
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

#ifndef INCLUDE_FLASHPTP_NETWORK_PHC_H_
#define INCLUDE_FLASHPTP_NETWORK_PHC_H_

#include <flashptp/common/includes.h>

#define FLASH_PTP_LINUX_PHC_IF                           "/dev/ptp"

namespace flashptp {
namespace network {

class PHC {
public:
    /*
     * Try to open the PTP hardware clock with the provided name (i.e., /dev/ptp0),
     * to read the capabilities of the clock and to determine the clock id to be used
     * for clock_adjtime or clock_gettime/clock_settime calls.
     */
    PHC(const std::string &name = "");
    ~PHC();

    /*
     * Return true, if the id of the PTP hardware clock has been successfully determined,
     * meaning that a connection to the PHC has been established and it can be read or
     * adjusted using clock_gettime, clock_settime and clock_adjtime API functions.
     */
    inline bool valid() const { return _id != CLOCK_REALTIME; }

    inline const std::string &name() const { return _name; }
    inline clockid_t id() const { return _id; }

    void exit();
    void init(const std::string &name = "");

private:
    std::string _name;

    clockid_t _id{ CLOCK_REALTIME };
    int _sock{ -1 };
    struct ptp_clock_caps _caps{ 0 };
};

}
}

#endif /* INCLUDE_FLASHPTP_NETWORK_PHC_H_ */
