/*
 * @file passThrough.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * This class implements a very simple pass-through of completed Sync Request/
 * Sync Response sequences (@see Calculation.h). As soon as a sequence has been
 * completed and inserted (size = 1), the measured offset and drift can be used
 * by an adjustment algorithm to perform a clock adjustment. This "calculation"
 * should only be used for very good connections (i.e., full path PTP support).
 *
 * The class is header-only.
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

#ifndef INCLUDE_FLASHPTP_CALCULATION_PASSTHROUGH_H_
#define INCLUDE_FLASHPTP_CALCULATION_PASSTHROUGH_H_

#include <flashptp/calculation/calculation.h>
#include <flashptp/client/sequence.h>

namespace flashptp {
namespace calculation {

class PassThrough : public Calculation {
public:
    inline PassThrough() : Calculation(CalculationType::passThrough, 1) { }

    /*
     * Pass through the calculated offset of the last complete sequence
     * that has been added and calculate the drift using the current and the
     * previous sequence (if available).
     *
     * Indicate, that a new adjustment is to be applied as soon as the drift
     * has been calculated (at least two sequences have been added).
     */
    inline virtual void calculate()
    {
        std::unique_lock ul(_mutex);

        _valid = _sequences.size() >= 1;
        if (!_valid)
            return;

        _delay = _sequences.back()->meanPathDelay();
        _offset = _sequences.back()->offset();
        if (_prevSeqValid) {
            _drift = (double)(_sequences.back()->offset() - _prevSeqOffset) /
                    (double)(_sequences.back()->t1() - _prevSeqTimestamp);
            _adjustment = true;
        }
        else {
            _drift = 0;
            _adjustment = false;
        }
    }
};

}
}

#endif /* INCLUDE_FLASHPTP_CALCULATION_PASSTHROUGH_H_ */
