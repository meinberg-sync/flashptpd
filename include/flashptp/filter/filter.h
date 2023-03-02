/*
 * @file filter.h
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
 *
 * The abstract class Filter provides a pattern for filtering algorithms
 * within flashPTP. If you want to add a new algorithm, all you need to do
 * is provide a derived class that implements the pure virtual method filter,
 * add an appropriate FilterType to the enum class specified below and adapt
 * the static member functions of the Filter class to your new type.
 *
 * The (configurable) size of a filter algorithm specifies, how many
 * Sync Request/Sync Response sequences have to be completed and inserted,
 * before the filter is full and the filtering can be performed. The number
 * of sequences that shall be selected by the filter can be configured
 * as well (pick, default = 1).
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

#ifndef INCLUDE_FLASHPTP_FILTER_FILTER_H_
#define INCLUDE_FLASHPTP_FILTER_FILTER_H_

#include <flashptp/common/defines.h>

#define FLASH_PTP_JSON_CFG_FILTER_TYPE                              "type"
#define FLASH_PTP_JSON_CFG_FILTER_SIZE                              "size"
#define FLASH_PTP_JSON_CFG_FILTER_PICK                              "pick"

namespace flashptp {

namespace client {
class Sequence;
}

namespace filter {

/*
 * Enumeration class of currently provided filter algorithms.
 * Add a type here, if you implement (derive from Filter) your own algorithm.
 * Remember to adapt the following static member functions of the Filter class:
 *      - typeToStr
 *      - typeFromStr
 *      - make
 *      - validateConfig (if needed)
 */
enum class FilterType
{
    invalid,
    luckyPacket,
    medianOffset,
    max = medianOffset
};

class Filter {
public:
    inline Filter(FilterType type = FilterType::invalid) : _type{ type } { }
    virtual inline ~Filter() { clear(); }

    static const char *typeToStr(FilterType t);
    static FilterType typeFromStr(const char *str);

    // Instantiate one of the implementations of the Filter class based on the provided config
    static Filter *make(const Json &config);
    static bool validateConfig(const Json &config, std::vector<std::string> *errs);
    void setConfig(const Json &config);

    // Returns the size of the filter that needs to be reached before the filtering can be performed
    inline unsigned size() const { return _size; }
    // Returns the number of sequences that are to be selected by the filter algorithm
    inline unsigned pick() const { return _pick; }

    void insert(client::Sequence *seq);
    void clear();

    inline bool empty() const { return _unfiltered.size() == 0; }
    inline bool full() const { return _unfiltered.size() >= _size; }

    // Pure virtual method to be implemented by deriving filter algorithm implementations.
    virtual void filter(std::deque<client::Sequence*> &filtered) = 0;

protected:
    FilterType _type{ FilterType::invalid };
    unsigned _size{ FLASH_PTP_DEFAULT_FILTER_SIZE };
    unsigned _pick{ FLASH_PTP_DEFAULT_FILTER_PICK };

    std::deque<client::Sequence*> _unfiltered;
};

}
}

#endif /* INCLUDE_FLASHPTP_FILTER_FILTER_H_ */
