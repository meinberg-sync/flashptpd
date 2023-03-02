/*
 * @file adjtimex.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * This class implements an adjustment algorithm (@see Adjustment.h) for the
 * system clock (CLOCK_REALTIME) using the standard Linux API function adjtimex.
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

#ifndef INCLUDE_FLASHPTP_ADJUSTMENT_ADJTIMEX_H_
#define INCLUDE_FLASHPTP_ADJUSTMENT_ADJTIMEX_H_

#include <flashptp/adjustment/adjustment.h>

namespace flashptp {
namespace adjustment {

class Adjtimex : public Adjustment {
public:
    inline Adjtimex() : Adjustment(AdjustmentType::adjtimex, FLASH_PTP_SYSTEM_CLOCK_NAME, CLOCK_REALTIME) {  }

    bool adjust(std::vector<client::Server*> &servers);

    /*
     * Clear the calculations of all servers after an adjustment has been applied
     * to make sure that measurements that have been used for this adjustment
     * will not be used for the next adjustment, again.
     */
    void finalize(const std::vector<client::Server*> &servers);
};

}
}

#endif /* INCLUDE_FLASHPTP_ADJUSTMENT_ADJTIMEX_H_ */
