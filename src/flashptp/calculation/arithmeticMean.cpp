/*
 * @file arithmeticMean.cpp
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

#include <flashptp/calculation/arithmeticMean.h>
#include <flashptp/client/sequence.h>

namespace flashptp {
namespace calculation {

void ArithmeticMean::calculate()
{
    std::unique_lock ul(_mutex);
    if (_sequences.size() < 2)
        return;

    int64_t size = _sequences.size();

    _delay = 0;
    _offset = 0;
    _drift = 0;
    for (unsigned i = 0; i < size; ++i) {
        _delay += _sequences[i]->meanPathDelay();
        _offset += _sequences[i]->offset();
        if (i >= 1)
            _drift += (double)(_sequences[i]->offset() - _sequences[i - 1]->offset()) /
                   (double)(_sequences[i]->t1() - _sequences[i - 1]->t1());
    }

    _delay /= size;
    _offset /= size;
    _drift /= (double)(size - 1);
    _valid = true;

    _adjustment = _sequences.size() >= _size;
}

}
}
