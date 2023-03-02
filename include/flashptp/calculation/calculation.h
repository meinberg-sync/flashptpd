/*
 * @file calculation.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The abstract class Calculation provides a pattern for calculation algorithm
 * implementations (delay, offset, drift) within flashPTP. If you want to add
 * a new algorithm, all you need to do is provide a derived class that implements
 * the pure virtual method calculate, add an appropriate CalculationType to the
 * enum class specified below and adapt the static member functions of the
 * Calculation class to your new type.
 *
 * The (configurable) size of a calculation algorithm specifies, how many
 * Sync Request/Sync Response sequences have to be completed and inserted,
 * before a calculation can be performed. A compensation value (nanoseconds)
 * can be configured to compensate for known asymmetries.
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

#ifndef INCLUDE_FLASHPTP_CALCULATION_CALCULATION_H_
#define INCLUDE_FLASHPTP_CALCULATION_CALCULATION_H_

#include <flashptp/common/defines.h>

#define FLASH_PTP_JSON_CFG_CALCULATION_TYPE                         "type"
#define FLASH_PTP_JSON_CFG_CALCULATION_SIZE                         "size"
#define FLASH_PTP_JSON_CFG_CALCULATION_COMPENSATION_VALUE           "compensationValue"

namespace flashptp {

namespace client {
class Sequence;
}

namespace calculation {

/*
 * Enumeration class of currently provided calculation algorithms.
 * Add a type here, if you implement (derive from Calculation) your own algorithm.
 * Remember to adapt the following static member functions of the Calculation class:
 *      - typeToStr
 *      - typeFromStr
 *      - make
 *      - validateConfig (if needed)
 */
enum class CalculationType
{
    invalid,
    passThrough,
    arithmeticMean,
    max = arithmeticMean
};

class Calculation {
public:
    inline Calculation(CalculationType type = CalculationType::invalid,
            unsigned size = FLASH_PTP_DEFAULT_CALCULATION_SIZE) : _type{ type }, _size { size } { }
    virtual ~Calculation();

    static const char *typeToStr(CalculationType t);
    static CalculationType typeFromStr(const char *str);

    // Instantiate one of the implementations of the Calculation class based on the provided config
    static Calculation *make(const Json &config);
    static bool validateConfig(const Json &config, std::vector<std::string> *errs);
    void setConfig(const Json &config);

    inline CalculationType type() const { return _type; }

    inline unsigned numSequences() const { std::shared_lock sl(_mutex); return _sequences.size(); }
    inline unsigned size() const { return _size; }

    void insert(client::Sequence *seq);
    void clear();
    void remove();

    // Returns true if the configured amount of complete sequences has been reached.
    inline bool fullyLoaded() const { std::shared_lock sl(_mutex); return _sequences.size() >= _size; }

    // Pure virtual method to be implemented by deriving calculation algorithm implementations.
    virtual void calculate() = 0;

    /*
     * Returns the timespan (ns) between the first and the last complete sequence that have
     * been added to the calculation implementation, i.e., the complete measurement window duration.
     */
    virtual int64_t windowDuration() const;
    // Returns the current rate (s) in which complete sequences are being added to the calculation
    virtual double sampleRate() const;

    // Indicates, whether the current values returned for delay, offset and drift are valid or not
    inline bool valid() const { std::shared_lock sl(_mutex); return _valid; }
    inline int64_t delay() const { std::shared_lock sl(_mutex); return _delay; }
    inline int64_t offset() const { std::shared_lock sl(_mutex); return _offset - _compensationValue; }
    inline double drift() const { std::shared_lock sl(_mutex); return _drift; }

    // Indicates, whether a new adjustment is to be applied with the calculated offset and drift
    bool hasAdjustment() const { std::shared_lock sl(_mutex); return _valid && _adjustment; }
    inline void setAdjustment(bool adjustment) { std::unique_lock sl(_mutex); _adjustment = adjustment; }

    inline PTPTimestampLevel timestampLevel() const { std::shared_lock sl(_mutex); return _timestampLevel; }

    virtual void reset();

protected:
    void clearLocked();

    CalculationType _type{ CalculationType::invalid };
    unsigned _size{ FLASH_PTP_DEFAULT_CALCULATION_SIZE };
    int64_t _compensationValue{ 0 };

    mutable std::shared_mutex _mutex;
    // The following members need to be read/write protected by the above mutex
    std::vector<client::Sequence*> _sequences;
    PTPTimestampLevel _timestampLevel{ PTPTimestampLevel::invalid };

    bool _valid{ false };
    int64_t _delay{ 0 };
    int64_t _offset{ 0 };
    double _drift{ 0 };

    bool _adjustment{ false };

    bool _prevSeqValid{ false };
    PTP2Timestamp _prevSeqTimestamp;
    int64_t _prevSeqOffset{ 0 };
};

}
}

#endif /* INCLUDE_FLASHPTP_CALCULATION_CALCULATION_H_ */
