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
#include "../../../include/flashptp/selection/btca.h"

namespace flashptp {
namespace selection {

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
}

std::vector<client::Server*> Selection::preprocess(const std::vector<client::Server*> servers, clockid_t clockID)
{
    std::vector<client::Server*> v;
    struct sockaddr_ll saddr_ll;

    for (auto *s: servers) {
        if (s->state() < client::ServerState::ready || s->clockID() != clockID)
            continue;

        if (llabs(s->calculation()->delay()) > _delayThreshold) {
            if (s->state() != client::ServerState::falseticker) {
                cppLog::debugf("Consider server %s as %s due to delay threshold exceedance (%s > %s)",
                        s->dstAddress().str().c_str(), client::Server::stateToLongStr(client::ServerState::falseticker),
                        nanosecondsToStr(llabs(s->calculation()->delay())).c_str(),
                        nanosecondsToStr(_delayThreshold).c_str());
                s->setState(client::ServerState::falseticker);
            }
            continue;
        }

        s->setState(client::ServerState::ready);
        v.push_back(s);
    }
    return v;
}

void Selection::postprocess(const std::vector<client::Server*> &servers, clockid_t clockID)
{
    for (auto *s: servers)
        s->setState(clockID == CLOCK_REALTIME ? client::ServerState::syspeer : client::ServerState::used);
}

}
}
