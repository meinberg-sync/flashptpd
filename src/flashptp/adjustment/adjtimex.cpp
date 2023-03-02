/*
 * @file adjtimex.cpp
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

#include <flashptp/adjustment/adjtimex.h>
#include <flashptp/client/server.h>

namespace flashptp {
namespace adjustment {

bool Adjtimex::adjust(std::vector<client::Server*> &servers)
{
    if (!initAdj(servers))
        return false;

    _timeAddend = 0;
    for (unsigned i = 0; i < servers.size(); ++i)
        _timeAddend += servers[i]->calculation()->offset();
    _timeAddend /= (int64_t)servers.size();

    int rc = -1;

    if (llabs(_timeAddend) >= FLASH_PTP_ADJUSTMENT_STEP_LIMIT_DEFAULT) {
        struct timespec ts;
        int64_t ns;
        clock_gettime(_clockID, &ts);
        ns = (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec + _timeAddend;
        ts.tv_sec = ns / 1000000000LL;
        ts.tv_nsec = ns % 1000000000LL;
        rc = clock_settime(_clockID, &ts);
        if (rc >= 0)
            cppLog::infof("Step Threshold (%s) exceeded - Stepped %s clock by %s, successfully",
                    nanosecondsToStr(FLASH_PTP_ADJUSTMENT_STEP_LIMIT_DEFAULT).c_str(), _clockName.c_str(),
                    nanosecondsToStr(_timeAddend).c_str());
        else
            cppLog::errorf("%s clock could not be adjusted: %s (%d)",
                    _clockName.c_str(), strerror(errno), errno);
    }
    else {
        struct timex tx;
        memset(&tx, 0, sizeof(tx));
        rc = adjtimex(&tx);
        if (rc < 0) {
            cppLog::errorf("Failed to read adjustment status of %s clock: %s (%d)",
                    _clockName.c_str(), strerror(errno), errno);
            return false;
        }

        tx.modes |= ADJ_OFFSET;
        tx.modes |= ADJ_STATUS;
        tx.modes |= ADJ_NANO;
        tx.status |= STA_PLL;
        tx.status |= STA_NANO;
        tx.status &= ~STA_RONLY;
        tx.status &= ~STA_FREQHOLD;
        tx.offset = _timeAddend;
        rc = adjtimex(&tx);
        if (rc >= 0)
            cppLog::debugf("Adjusted %s clock (adjtimex) by %s, successfully",
                    _clockName.c_str(), nanosecondsToStr(_timeAddend).c_str());
        else
            cppLog::errorf("%s clock could not be adjusted (adjtimex): %s (%d)",
                    _clockName.c_str(), strerror(errno), errno);
    }

    return rc >= 0;
}

void Adjtimex::finalize(const std::vector<client::Server*> &servers)
{
    Adjustment::finalize(servers);
    for (unsigned i = 0; i < servers.size(); ++i)
        servers[i]->calculation()->clear();
}

}
}
