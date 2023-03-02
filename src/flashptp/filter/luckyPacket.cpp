/*
 * @file luckyPacket.cpp
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

#include <flashptp/filter/luckyPacket.h>
#include <flashptp/client/sequence.h>

namespace flashptp {
namespace filter {

void LuckyPacket::filter(std::deque<client::Sequence*> &filtered)
{
    if (_unfiltered.size() < _size)
        return;

    unsigned i, j;
    int64_t delay;

    std::deque<client::Sequence*> output;
    while (output.size() < _pick) {
        delay = INT64_MAX;
        for (i = 0; i < _unfiltered.size(); ++i) {
            if (llabs(_unfiltered[i]->meanPathDelay()) < delay) {
                delay = llabs(_unfiltered[i]->meanPathDelay());
                j = i;
            }
        }

        if (delay != INT64_MAX) {
            output.push_back(_unfiltered[j]);
            _unfiltered.erase(_unfiltered.begin() + j);
        }
        else
            break;
    }

    while (_unfiltered.size()) {
        delete _unfiltered.front();
        _unfiltered.pop_front();
    }

    filtered.insert(filtered.end(), output.begin(), output.end());
}

}
}
