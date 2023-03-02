/*
 * @file phc.cpp
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

#include <flashptp/network/phc.h>

namespace flashptp {
namespace network {

PHC::PHC(const std::string &name)
{
    _name = name;
    init();
}

PHC::~PHC()
{
    exit();
}

void PHC::exit()
{
    _id = CLOCK_REALTIME;
    if (_sock >= 0) {
        close(_sock);
        _sock = -1;
    }
    memset(&_caps, 0, sizeof(_caps));
}

void PHC::init(const std::string &name)
{
    exit();
    if (!name.empty())
        _name = name;
    if (_name.empty())
        return;

    _sock = open(_name.c_str(), O_RDWR);
    if (_sock < 0) {
        cppLog::warningf("Could not open %s for read/write: %s (%d)", _name.c_str(), strerror(errno), errno);
        return;
    }

    if (ioctl(_sock, PTP_CLOCK_GETCAPS, &_caps) == 0) {
        struct timex tx;
        _id = (((unsigned int)~_sock) << 3) | 3;
        memset(&tx, 0, sizeof(tx));
        clock_adjtime(_id, &tx);
    }
    else
        cppLog::warningf("Could not get PTP capabilities for %s: %s (%d)", _name.c_str(),
                strerror(errno), errno);

    if (_id == CLOCK_REALTIME)
        exit();
}

}
}
