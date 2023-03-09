/*
 * @file selection.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * The abstract class Selection provides a pattern for server selection algorithm
 * implementations within flashPTP. If you want to add a new algorithm, all you
 * need to do is provide a derived class that implements the pure virtual method
 * select, add an appropriate SelectType to the enum class specified below and
 * adapt the static member functions of the Selection class to your new type.
 *
 * The number of servers, that shall be selected for clock adjustment can be
 * configured (pick). If more than one server is being selected, the adjustment
 * algorithm averages the measured offset and drift values.
 *
 * A server is considered as "falseticker" and will not be selected, if one of
 * the following checks is true:
 *      - "noSelect" option is true for the server
 *      - the configured delay threshold (default: 1.5 seconds) is exceeded
 *      - the server does not have measurement points within the calculated
 *        intersection interval (@see detectTruechimers)
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

#ifndef INCLUDE_FLASHPTP_SELECTION_SELECTION_H_
#define INCLUDE_FLASHPTP_SELECTION_SELECTION_H_

#include <flashptp/common/defines.h>

#define FLASH_PTP_JSON_CFG_SELECTION_TYPE                           "type"
#define FLASH_PTP_JSON_CFG_SELECTION_PICK                           "pick"
#define FLASH_PTP_JSON_CFG_SELECTION_DELAY_THRESHOLD                "delayThreshold"
#define FLASH_PTP_JSON_CFG_SELECTION_INTERSECTION_PADDING           "intersectionPadding"

namespace flashptp {

namespace client {
class Server;
}

namespace selection {

/*
 * Enumeration class of currently provided selection algorithms.
 * Add a type here, if you implement (derive from Selection) your own algorithm.
 * Remember to adapt the following static member functions of the Selection class:
 *      - typeToStr
 *      - typeToLongStr
 *      - typeFromStr
 *      - make
 *      - validateConfig (if needed)
 */
enum class SelectionType
{
    invalid,
    stdDev,
    btca,
    max = btca
};

class Selection {
public:
    inline Selection(SelectionType type = SelectionType::invalid) : _type{ type } { }
    virtual inline ~Selection() { }

    static const char *typeToStr(SelectionType t);
    static const char *typeToLongStr(SelectionType t);
    static SelectionType typeFromStr(const char *str);

    // Instantiate one of the implementations of the Selection class based on the provided config
    static Selection *make(const Json &config);
    static bool validateConfig(const Json &config, std::vector<std::string> *errs);
    void setConfig(const Json &config);

    // Pure virtual method to be implemented by deriving selection algorithm implementations.
    virtual std::vector<client::Server*> select(const std::vector<client::Server*> servers,
            clockid_t clockID = CLOCK_REALTIME) = 0;

protected:
    /*
     * Pre-select servers from the provided vector that are at least in "ready" state,
     * use the correct clock for calculation and are considered to be truechimers (@see detectTruechimers).
     */
    std::vector<client::Server*> preprocess(const std::vector<client::Server*> &servers, clockid_t clockID);

    // Set the state of selected servers to "Selected"
    void postprocess(const std::vector<client::Server*> &servers, clockid_t clockID);

    SelectionType _type{ SelectionType::invalid };
    unsigned _pick{ FLASH_PTP_DEFAULT_SELECTION_PICK };
    uint64_t _delayThreshold{ FLASH_PTP_DEFAULT_SELECTION_DELAY_THRESHOLD };
    uint64_t _intersectionPadding{ 0 };

private:
    /*
     * Try to differ between truechimers and falsetickers. If possible, calculate the intersection
     * interval and consider all servers that have points within that interval as truechimers.
     */
    std::vector<client::Server*> detectTruechimers(const std::vector<client::Server*> &servers, clockid_t clockID);
};

}
}

#endif /* INCLUDE_FLASHPTP_SELECTION_SELECTION_H_ */
