/*
 * @file luckyPacket.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * This class implements a lucky packet filter algorithm (@see Filter.h).
 * As soon as the configured amount (size) of Sync Request/Sync Response sequences
 * have been completed, the filter selects the configured amount (pick) of
 * sequences with the lowest measured path delay (lucky packets). Effectively
 * this helps select for those packets which had the least interference through
 * the network. The filter should only be used with small request intervals
 * (at least 8 requests per second).
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

#ifndef INCLUDE_FLASHPTP_FILTER_LUCKYPACKET_H_
#define INCLUDE_FLASHPTP_FILTER_LUCKYPACKET_H_

#include <flashptp/filter/filter.h>

namespace flashptp {
namespace filter {

class LuckyPacket : public Filter {
public:
    inline LuckyPacket() : Filter(FilterType::luckyPacket) { }

    // Select the configured amount of sequences with the lowest measured path delay
    void filter(std::deque<client::Sequence*> &filtered);
};

}
}

#endif /* INCLUDE_FLASHPTP_FILTER_LUCKYPACKET_H_ */
