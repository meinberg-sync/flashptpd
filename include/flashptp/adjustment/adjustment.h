/*
 * @file adjustment.h
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
 *
 * The abstract class Adjustment provides a pattern for adjustment algorithm
 * implementations within flashPTP. If you want to add a new algorithm,
 * all you need to do is provide a derived class that implements the pure
 * virtual method adjust, add an appropriate AdjustmentType to the enum class
 * specified below and adapt the static member functions of the Adjustment
 * class to your new type.
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

#ifndef INCLUDE_FLASHPTP_ADJUSTMENT_ADJUSTMENT_H_
#define INCLUDE_FLASHPTP_ADJUSTMENT_ADJUSTMENT_H_

#include <flashptp/common/defines.h>

#define FLASH_PTP_JSON_CFG_ADJUSTMENT_TYPE                          "type"
#define FLASH_PTP_JSON_CFG_ADJUSTMENT_CLOCK                         "clock"

#ifndef ADJ_SETOFFSET
    #define ADJ_SETOFFSET 0x0100
#endif

#define FLASH_PTP_ADJUSTMENT_STEP_LIMIT_DEFAULT                     500000000ULL
#define FLASH_PTP_ADJUSTMENT_FREQ_LIMIT                             32768000

namespace flashptp {

namespace client {
class Server;
}

namespace adjustment {

/*
 * Enumeration class of currently provided adjustment algorithms.
 * Add a type here, if you implement (derive from Adjustment) your own algorithm.
 * Remember to adapt the following static member functions of the Adjustment class:
 *      - typeToStr
 *      - typeFromStr
 *      - make
 *      - validateConfig (if needed)
 */
enum class AdjustmentType
{
    invalid,
    adjtimex,
    pidController,
    max = pidController
};


class Adjustment {
public:
    inline Adjustment(AdjustmentType type = AdjustmentType::invalid, const std::string &clockName = "",
            clockid_t clockID = -1) { _type = type; _clockName = clockName; _clockID = clockID; }
    virtual inline ~Adjustment() { }

    static const char *typeToStr(AdjustmentType t);
    static AdjustmentType typeFromStr(const char *str);

    // Instantiate one of the implementations of the Adjustment class based on the provided config
    static Adjustment *make(const Json &config);
    static bool validateConfig(const Json &config, std::vector<std::string> *errs);
    virtual void setConfig(const Json &config);

    inline clockid_t clockID() const { return _clockID; }

    /*
     * If the clock to be adjusted is not the system clock (CLOCK_REALTIME), try to find out
     * the clock id of the appropriate PTP hardware clock (PHC).
     */
    virtual bool prepare();

    // Pure virtual method to be implemented by deriving adjustment algorithm implementations.
    virtual bool adjust(std::vector<client::Server*> &servers) = 0;

    /*
     * Do what needs to be done after an adjustment has been applied.
     * Everything the standard implementation does is to reset the adjustment flag of
     * the calculation object of each server that has been used for the adjustment.
     */
    virtual void finalize(const std::vector<client::Server*> &servers);

protected:
    /*
     * Check if all servers that have been selected for the adjustment have a new adjustment value
     * and if the server uses the correct clock for the calculation
     */
    bool initAdj(std::vector<client::Server*> &servers);

    AdjustmentType _type{ AdjustmentType::invalid };

    std::string _clockName;
    clockid_t _clockID{ -1 };

    int64_t _timeAddend{ 0 };
    double _freqAddend{ 0 };
    double _freqAggregate{ 0 };
};

}
}

#endif /* INCLUDE_FLASHPTP_ADJUSTMENT_ADJUSTMENT_H_ */
