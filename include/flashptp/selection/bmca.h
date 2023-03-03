/*
 * @file bmca.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * This class implements the IEEE1588 best master clock algorithm as server selection
 * algorithm (@see Selection.h). Servers that shall be taken into consideration
 * by this algorithm need to request the BMCA_COMPARISON_DS in the Sync Request TLV.
 * The algorithm will then select the configured amount of servers (pick) with the
 * best PTP quality parameters.
 *
 * At the moment, the BMCA quality parameters in flashptpd server mode can only
 * be specified via configuration. Therefore, the outcome of the BMCA selection
 * algorithm is kind of predefined and does not really make sense. If you are using
 * flashptpd on the server-side and automatic BMCA parameter detection still has not
 * been implemented, you should use standard deviation selection (@see stdDev.h), instead.
 *
 * If you want to use BMCA selection, anyway, please note that you need to enable
 * requesting FlashPTPServerStateDS from each server by setting the appropriate
 * config property "serverStateSpan" to the span between consecutive request packets
 * that shall include a request for the data set.
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

#ifndef INCLUDE_FLASHPTP_SELECTION_BMCA_H_
#define INCLUDE_FLASHPTP_SELECTION_BMCA_H_

#include <flashptp/selection/selection.h>

namespace flashptp {
namespace selection {

class BMCA : public Selection {
public:
    inline BMCA() : Selection(SelectionType::bmca) { }

    /*
     * Static member that compares the provided server state data sets.
     * Returns <0 if ds1 wins, >0 if ds2 wins and 0 if the data sets are equal.
     */
    static int compare(const FlashPTPServerStateDS &ds1, const FlashPTPServerStateDS &ds2);

    // Select the configured amount of servers with the best BMCA (clock quality) parameters.
    std::vector<client::Server*> select(const std::vector<client::Server*> servers,
            clockid_t clockID = CLOCK_REALTIME);
};

}
}

#endif /* INCLUDE_FLASHPTP_SELECTION_BMCA_H_ */
