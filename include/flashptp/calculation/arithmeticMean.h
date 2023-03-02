/*
 * @file arithmeticMean.h
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
 *
 * This class implements an arithmetic mean calculation algorithm (@see Calculation.h).
 * As soon as the configured amount (size) of Sync Request/Sync Response sequences
 * have been completed, the algorithm calculates the arithmetic mean of all measured
 * delay, offset and drift values. This allows to have kind of a sliding window of
 * measurements and helps to reduce measurement errors (i.e., introduced by packet
 * delay variation (jitter) or inaccurate timestamping). The sample rate and window
 * duration depends on the request interval and possibly used filter algorithms.
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

#ifndef INCLUDE_FLASHPTP_CALCULATION_ARITHMETICMEAN_H_
#define INCLUDE_FLASHPTP_CALCULATION_ARITHMETICMEAN_H_

#include <flashptp/calculation/calculation.h>

namespace flashptp {
namespace calculation {

class ArithmeticMean : public Calculation {
public:
    inline ArithmeticMean() : Calculation(CalculationType::arithmeticMean) { }

    /*
     * Calculate the arithmetic mean of delay, offset and drift of all
     * complete sequences that have been added to the calculation.
     * We need at least two complete sequences.
     *
     * Indicate, that a new adjustment is to be applied as soon as the
     * configured amount (size) of sequences have been added.
     */
    virtual void calculate();
};

}
}

#endif /* INCLUDE_FLASHPTP_CALCULATION_ARITHMETICMEAN_H_ */
