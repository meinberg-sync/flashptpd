/*
 * @file medianOffset.cpp
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

#include <flashptp/filter/medianOffset.h>
#include <flashptp/client/sequence.h>

namespace flashptp {
namespace filter {

void MedianOffset::filter(std::deque<client::Sequence*> &filtered)
{
    if (_unfiltered.size() < _size)
        return;

    std::sort(_unfiltered.begin(), _unfiltered.end(),
            [](const client::Sequence *a, const client::Sequence *b) { return a->offset() < b->offset(); });

    std::deque<client::Sequence*> output;
    int mIndex;
    while (output.size() < _pick && _unfiltered.size() > 2) {
        mIndex = _unfiltered.size() / 2;
        output.push_back(_unfiltered[mIndex]);
        _unfiltered.erase(_unfiltered.begin() + mIndex);
    }

    while (_unfiltered.size()) {
        delete _unfiltered.front();
        _unfiltered.pop_front();
    }

    filtered.insert(filtered.end(), output.begin(), output.end());
}

}
}
