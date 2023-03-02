/*
 * @file stdDev.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * This class implements a standard deviation selection algorithm (@see Selection.h).
 * The algorithm selects the configured amount of servers (pick) with the best
 * (lowest) standard deviation in the measured offsets.
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

#ifndef INCLUDE_FLASHPTP_SELECTION_STD_DEV_H_
#define INCLUDE_FLASHPTP_SELECTION_STD_DEV_H_

#include <flashptp/selection/selection.h>

namespace flashptp {
namespace selection {

class StdDev : public Selection {
public:
    inline StdDev() : Selection(SelectionType::stdDev) { }

    // Select the configured amount of servers with the best (lowest) standard deviation in the measured offsets.
    std::vector<client::Server*> select(const std::vector<client::Server*> servers,
            clockid_t clockID = CLOCK_REALTIME);
};

}
}

#endif /* INCLUDE_FLASHPTP_SELECTION_STD_DEV_H_ */
