/*
 * @file selection.cpp
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
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

#include <flashptp/selection/selection.h>
#include <flashptp/selection/stdDev.h>
#include <flashptp/client/clientMode.h>
#include <flashptp/selection/btca.h>

namespace flashptp {
namespace selection {

struct IntersectionGroup {
    std::vector<client::Server*> _servers;
    int64_t _padsize{ 0 };
    int64_t _maxOffsetDifference{ 0 };

    int64_t _cumulatedMin{ 0 };
    int64_t _cumulatedMax{ 0 };
    int64_t _cumulatedOffset{ 0 };

    int64_t _min{ 0 };
    int64_t _max{ 0 };
    int64_t _offset{ 0 };

    inline IntersectionGroup(client::Server *srv, int64_t padsize = 0, int64_t maxOffsetDifference = 0,
            int64_t min = INT64_MIN, int64_t max = INT64_MAX)
    {
        _padsize = padsize;
        _maxOffsetDifference = maxOffsetDifference;
        addServer(srv, min, max);
    }

    inline bool contains(int64_t min, int64_t max, int64_t offset) const
    {
        if (_maxOffsetDifference > 0 &&
            llabs(_offset - offset) > _maxOffsetDifference)
            return false;
        else
            return min < _max && max > _min;
    }

    inline void addServer(client::Server *srv, int64_t min = INT64_MIN, int64_t max = INT64_MAX)
    {
        _servers.push_back(srv);
        if (min == INT64_MIN)
            min = srv->calculation()->minOffset();
        if (max == INT64_MAX)
            max = srv->calculation()->maxOffset();
        _cumulatedMin += min;
        _cumulatedMax += max;
        _cumulatedOffset += srv->calculation()->offset();
        _min = _cumulatedMin / (int64_t)_servers.size();
        _max = _cumulatedMax / (int64_t)_servers.size();
        _offset = _cumulatedOffset / (int64_t)_servers.size();
        pad();
    }

    inline void pad()
    {
        if (llabs(_max - _min) >= _padsize)
            return;

        int64_t mean = (_max + _min) / 2;
        _min = mean - (_padsize / 2);
        _max = mean + (_padsize / 2);
    }
};

const char *Selection::typeToStr(SelectionType t)
{
    switch (t) {
    case SelectionType::stdDev: return "stdDev";
    case SelectionType::btca: return "btca";
    default: return "invalid";
    }
}

const char *Selection::typeToLongStr(SelectionType t)
{
    switch (t) {
    case SelectionType::stdDev: return "bestStandardDeviation";
    case SelectionType::btca: return "bestTimeTransmitterClock";
    default: return "invalid";
    }
}

SelectionType Selection::typeFromStr(const char *str)
{
    for (int i = (int)SelectionType::invalid + 1; i <= (int)SelectionType::max; ++i) {
        if (strcasecmp(str, Selection::typeToStr((SelectionType)i)) == 0 ||
            strcasecmp(str, Selection::typeToLongStr((SelectionType)i)) == 0)
            return (SelectionType)i;
    }
    return SelectionType::invalid;
}

Selection *Selection::make(const Json &config)
{
    Selection *sel = nullptr;
    SelectionType type = Selection::typeFromStr(
            config.find(FLASH_PTP_JSON_CFG_SELECTION_TYPE)->get<std::string>().c_str());

    switch (type) {
    case SelectionType::stdDev: sel = new StdDev(); break;
    case SelectionType::btca: sel = new BTCA(); break;
    default: break;
    }

    if (sel)
        sel->setConfig(config);

    return sel;
}

bool Selection::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    if (!config.is_object()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION "\" " \
                "must be \"") + Json::object().type_name() + "\".");
        return false;
    }

    Json::const_iterator it;
    bool valid = true;
    SelectionType type = SelectionType::invalid;

    it = config.find(FLASH_PTP_JSON_CFG_SELECTION_TYPE);
    if (it == config.end()) {
        errs->push_back("\"" FLASH_PTP_JSON_CFG_SELECTION_TYPE "\" must be specified within \"" \
                FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION "\" objects.");
        valid = false;
    }
    else if (!it->is_string()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SELECTION_TYPE "\" within \""
                FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION "\" objects must be \"") + Json("").type_name() + "\".");
        valid = false;
    }
    else {
        type = Selection::typeFromStr(it->get<std::string>().c_str());
        if (type == SelectionType::invalid) {
            errs->push_back(std::string("\"") + it->get<std::string>() + "\" is not a valid \"" \
                    FLASH_PTP_JSON_CFG_SELECTION_TYPE "\" (" + enumClassToStr<SelectionType>(&Selection::typeToStr) +
                    ") within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION "\" objects.");
            valid = false;
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SELECTION_PICK);
    if (it != config.end()) {
        if (!it->is_number_unsigned()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SELECTION_PICK "\" " \
                    "within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION "\" must be \"") +
                    Json((unsigned)1).type_name() + "\".");
            valid = false;
        }
        else if (it->get<unsigned>() == 0) {
            errs->push_back(std::to_string(it->get<unsigned>()) + " is not a valid value (0 < n) " \
                    "for property \"" FLASH_PTP_JSON_CFG_SELECTION_PICK "\".");
            valid = false;
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SELECTION_DELAY_THRESHOLD);
    if (it != config.end()) {
        if (!it->is_number_unsigned()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SELECTION_DELAY_THRESHOLD "\" " \
                    "within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION "\" must be \"") +
                    Json((unsigned)1).type_name() + "\".");
            valid = false;
        }
        else if (it->get<unsigned>() == 0) {
            errs->push_back(std::to_string(it->get<unsigned>()) + " is not a valid value (0 < n) " \
                    "for property \"" FLASH_PTP_JSON_CFG_SELECTION_DELAY_THRESHOLD "\".");
            valid = false;
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SELECTION_INTERSECTION_PADDING);
    if (it != config.end() && !it->is_number_unsigned()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SELECTION_INTERSECTION_PADDING "\" " \
                "within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION "\" must be \"") +
                Json((unsigned)1).type_name() + "\".");
        valid = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_SELECTION_MAX_OFFSET_DIFFERENCE);
    if (it != config.end() && !it->is_number_unsigned()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SELECTION_MAX_OFFSET_DIFFERENCE "\" " \
                "within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION "\" must be \"") +
                Json((unsigned)1).type_name() + "\".");
        valid = false;
    }

    return valid;
}

void Selection::setConfig(const Json &config)
{
    Json::const_iterator it;

    it = config.find(FLASH_PTP_JSON_CFG_SELECTION_PICK);
    if (it != config.end())
        it->get_to(_pick);
    else
        _pick = FLASH_PTP_DEFAULT_SELECTION_PICK;

    it = config.find(FLASH_PTP_JSON_CFG_SELECTION_DELAY_THRESHOLD);
    if (it != config.end())
        it->get_to(_delayThreshold);
    else
        _delayThreshold = FLASH_PTP_DEFAULT_SELECTION_DELAY_THRESHOLD;

    it = config.find(FLASH_PTP_JSON_CFG_SELECTION_INTERSECTION_PADDING);
    if (it != config.end())
        it->get_to(_intersectionPadding);
    else
        _intersectionPadding = 0;

    it = config.find(FLASH_PTP_JSON_CFG_SELECTION_MAX_OFFSET_DIFFERENCE);
    if (it != config.end())
        it->get_to(_maxOffsetDifference);
    else
        _maxOffsetDifference = 0;
}

std::vector<client::Server*> Selection::detectTruechimers(const std::vector<client::Server*> &servers,
        clockid_t clockID)
{
    std::vector<struct IntersectionGroup> groups;
    std::vector<client::Server*> truechimers;
    std::vector<unsigned> bestGroups;
    struct IntersectionGroup *grp;
    client::Server *srv;

    int64_t min, max, next, lli, paddingSize, maxOffsDiff;
    unsigned i, j, k;
    bool create;

    if (servers.size() <= 2) {
        // Now way to differ between falsetickers and truechimers
        truechimers = servers;
        goto finish;
    }

    /*
     * Find the intersection interval and consider all servers that have points within that
     * interval as truechimers (@see https://www.eecis.udel.edu/~mills/ntp/html/select.html).
     *
     * The intersection interval is defined as the
     *     "smallest interval containing points from the largest number of correctness intervals"
     *
     * The correctness interval is defined as the difference between the minimum and maximum
     * measured offsets within the current measurement window.
     *
     * For very good connections, the intersection interval can be quite small. To avoid permanent
     * rebuilding of groups (leading to clock hopping), the size of the intersection interval is padded
     * to the (configurable) value of _intersectionPadding.
     *
     * While the NTP documentation linked above gives a good idea on how the intersection interval
     * detection basically works, we do it slightly different here. The intersection interval of a
     * group of possible truechimers is calculated as the (possibly padded) mean minimum and maximum
     * correctness values of all servers that belong to that group.
     */
    if (_intersectionPadding == 0) {
        if (clockID == CLOCK_REALTIME)
            paddingSize = FLASH_PTP_DEFAULT_SELECTION_INTERSECTION_PADDING_SW;
        else
            paddingSize = FLASH_PTP_DEFAULT_SELECTION_INTERSECTION_PADDING_HW;
    }
    else
        paddingSize = _intersectionPadding;

    /*
     * Do not group servers, that have a difference greater than the (configurable) _maxOffsetDifference
     * in their measured offsets (@see IntersectionGroup::contains).
     */
    if (_maxOffsetDifference == 0) {
        if (clockID == CLOCK_REALTIME)
            maxOffsDiff = FLASH_PTP_DEFAULT_SELECTION_MAX_OFFSET_DIFFERENCE_SW;
        else
            maxOffsDiff = FLASH_PTP_DEFAULT_SELECTION_MAX_OFFSET_DIFFERENCE_HW;
    }
    else
        maxOffsDiff = _maxOffsetDifference;

    /*
     * Find groups of servers sharing measurement points within their correctness intervals.
     * Servers can be a member of multiple groups.
     */
    for (i = 0; i < servers.size(); ++i) {
        create = true;
        srv = servers[i];
        min = srv->calculation()->minOffset();
        max = srv->calculation()->maxOffset();

        for (j = 0; j < groups.size(); ++j) {
            grp = &groups[j];
            if (grp->contains(min, max, srv->calculation()->offset())) {
                grp->addServer(srv, min, max);
                create = false;
            }
        }

        if (create)
            groups.emplace_back(srv, paddingSize, maxOffsDiff, min, max);
    }

    // Determine the maximum group size and store the index/indices of the appropriate group(s)
    max = 0;
    for (i = 0; i < groups.size(); ++i) {
        grp = &groups[i];
        if (grp->_servers.size() > max) {
            max = grp->_servers.size();
            bestGroups = { i };
        }
        else if (grp->_servers.size() == max)
            bestGroups.push_back(i);
    }

    grp = &groups[bestGroups.front()];

    /*
     * If there is only one group left, consider that group as the truechimers.
     * Otherwise, find the group with the smallest intersection interval.
     *
     * To avoid clock (group) hopping, check if the difference between the intersection intervals
     * is bigger than the configured intersection padding value.
     */
    if (bestGroups.size() == 1) {
        truechimers = grp->_servers;
        goto finish;
    }

    min = grp->_max - grp->_min;
    for (i = 1, j = 0; i < bestGroups.size(); ++i) {
        grp = &groups[bestGroups[i]];
        next = grp->_max - grp->_min;
        lli = llabs(next - min);
        if (lli > paddingSize) {
            // Difference between the intersection intervals is big enough to be considered
            if (next < min) {
                bestGroups.erase(bestGroups.begin() + j);
                j = --i;
            }
            else {
                bestGroups.erase(bestGroups.begin() + i);
                --i;
            }
        }
    }

    grp = &groups[bestGroups.front()];

    /*
     * If there is only one group left, consider that group as the truechimers.
     * Otherwise, find the group with the smallest mean standard deviation.
     *
     * To avoid clock (group) hopping, check if the difference between the mean standard deviation values
     * is bigger than the configured intersection padding value.
     */
    if (bestGroups.size() == 1) {
        truechimers = grp->_servers;
        goto finish;
    }

    min = 0;
    for (k = 0; k < grp->_servers.size(); ++k)
        min += grp->_servers[k]->stdDev();
    min /= (int64_t)grp->_servers.size();

    for (i = 1, j = 0; i < bestGroups.size(); ++i) {
        grp = &groups[bestGroups[i]];
        next = 0;
        for (k = 0; k < grp->_servers.size(); ++k)
            next += grp->_servers[k]->stdDev();
        next /= (int64_t)grp->_servers.size();
        lli = llabs(next - min);
        if (lli > paddingSize) {
            // Difference between the mean standard deviation values is big enough to be considered
            if (next < min) {
                bestGroups.erase(bestGroups.begin() + j);
                j = --i;
            }
            else {
                bestGroups.erase(bestGroups.begin() + i);
                --i;
            }
        }
    }

    grp = &groups[bestGroups.front()];

    /*
     * If there is only one group left, consider that group as the truechimers.
     * Otherwise, find the group with the smallest mean path delay.
     *
     * To avoid clock (group) hopping, check if the difference between the mean path delay values
     * is bigger than the configured intersection padding value.
     */
    if (bestGroups.size() == 1) {
        truechimers = grp->_servers;
        goto finish;
    }

    min = 0;
    for (k = 0; k < grp->_servers.size(); ++k)
        min += grp->_servers[k]->calculation()->delay();
    min /= (int64_t)grp->_servers.size();

    for (i = 1, j = 0; i < bestGroups.size(); ++i) {
        grp = &groups[bestGroups[i]];
        next = 0;
        for (k = 0; k < grp->_servers.size(); ++k)
            next += grp->_servers[k]->calculation()->delay();
        next /= (int64_t)grp->_servers.size();
        lli = llabs(next - min);
        if (lli > paddingSize) {
            // Difference between the mean path delay values is big enough to be considered
            if (next < min) {
                bestGroups.erase(bestGroups.begin() + j);
                j = --i;
            }
            else {
                bestGroups.erase(bestGroups.begin() + i);
                --i;
            }
        }
    }

    grp = &groups[bestGroups.front()];

    // Regardless of how many groups being left, consider the first group in the list as the truechimers.
    truechimers = grp->_servers;

finish:
    for (i = 0; i < servers.size(); ++i) {
        if (std::find(truechimers.begin(), truechimers.end(), servers[i]) == truechimers.end())
            servers[i]->setState(client::ServerState::falseticker);
    }

    for (i = 0; i < truechimers.size(); ++i)
        truechimers[i]->setState(client::ServerState::candidate);

    return truechimers;
}

std::vector<client::Server*> Selection::preprocess(const std::vector<client::Server*> &servers, clockid_t clockID)
{
    std::vector<client::Server*> v;
    client::Server *srv;
    unsigned i;

    // Find all Servers in "Ready" state that use the desired clock (clockID)
    for (i = 0; i < servers.size(); ++i) {
        srv = servers[i];
        if (srv->state() < client::ServerState::ready || srv->clockID() != clockID)
            continue;

        // Skip servers with "noSelect" option
        if (srv->noSelect()) {
            srv->setState(client::ServerState::falseticker);
            continue;
        }

        // Skip servers that have a calculated delay exceeding the configured delay threshold
        if (llabs(srv->calculation()->delay()) > _delayThreshold) {
            if (srv->state() != client::ServerState::falseticker) {
                cppLog::debugf("Consider server %s as %s due to delay threshold exceedance (%s > %s)",
                        srv->dstAddress().str().c_str(), client::Server::stateToLongStr(client::ServerState::falseticker),
                        nanosecondsToStr(llabs(srv->calculation()->delay())).c_str(),
                        nanosecondsToStr(_delayThreshold).c_str());
                srv->setState(client::ServerState::falseticker);
            }
            continue;
        }

        v.push_back(srv);
    }

    /*
     * Check, if all pre-selected servers have a new adjustment value.
     * If not, return an empty vector to indicate, that no new selection has been performed, yet.
     */
    for (i = 0; i < v.size(); ++i) {
        if (!v[i]->calculation()->hasAdjustment())
            return { };
    }

    /*
     * All pre-selected servers have a new adjustment value.
     * Reset their state to "Ready" (temporarily).
     */
    for (i = 0; i < v.size(); ++i)
        v[i]->setState(client::ServerState::ready);

    // Now find the truechimers from all "Ready" servers (setting their state to "Candidate")
    v = Selection::detectTruechimers(v, clockID);

    return v;
}

void Selection::postprocess(const std::vector<client::Server*> &servers, clockid_t clockID)
{
    for (auto *s: servers)
        s->setState(client::ServerState::selected);
}

}
}
