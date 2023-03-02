/*
 * @file adjustment.cpp
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

#include <flashptp/adjustment/adjustment.h>
#include <flashptp/adjustment/adjtimex.h>
#include <flashptp/adjustment/pidController.h>
#include <flashptp/client/clientMode.h>
#include <flashptp/network/network.h>

namespace flashptp {
namespace adjustment {

const char *Adjustment::typeToStr(AdjustmentType t)
{
    switch (t) {
    case AdjustmentType::adjtimex: return "adjtimex";
    case AdjustmentType::pidController: return "pidController";
    default: return "invalid";
    }
}

AdjustmentType Adjustment::typeFromStr(const char *str)
{
    for (int i = (int)AdjustmentType::invalid + 1; i <= (int)AdjustmentType::max; ++i) {
        if (strcasecmp(str, Adjustment::typeToStr((AdjustmentType)i)) == 0)
            return (AdjustmentType)i;
    }
    return AdjustmentType::invalid;
}

Adjustment *Adjustment::make(const Json &config)
{
    Adjustment *adj = nullptr;
    AdjustmentType type = Adjustment::typeFromStr(
            config.find(FLASH_PTP_JSON_CFG_ADJUSTMENT_TYPE)->get<std::string>().c_str());

    switch (type) {
    case AdjustmentType::adjtimex: adj = new Adjtimex(); break;
    case AdjustmentType::pidController: adj = new PIDController(); break;
    default: break;
    }

    if (adj)
        adj->setConfig(config);

    return adj;
}

bool Adjustment::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    if (!config.is_object()) {
        errs->push_back(std::string("Type of items within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" " \
                "must be \"") + Json::object().type_name() + "\".");
        return false;
    }

    Json::const_iterator it;
    bool valid = true;
    AdjustmentType type = AdjustmentType::invalid;

    it = config.find(FLASH_PTP_JSON_CFG_ADJUSTMENT_TYPE);
    if (it == config.end()) {
        errs->push_back(std::string("\"" FLASH_PTP_JSON_CFG_ADJUSTMENT_TYPE "\" must be specified within " \
                "items of \"") + FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS + "\".");
        valid = false;
    }
    else if (!it->is_string()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_ADJUSTMENT_TYPE "\" within " \
                "items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" must be \"") +
                Json("").type_name() + "\".");
        valid = false;
    }
    else {
        type = Adjustment::typeFromStr(it->get<std::string>().c_str());
        if (type == AdjustmentType::invalid) {
            errs->push_back(std::string("\"") + it->get<std::string>() + "\" is not a valid \"" \
                    FLASH_PTP_JSON_CFG_ADJUSTMENT_TYPE "\" (" + enumClassToStr<AdjustmentType>(&Adjustment::typeToStr) +
                    ") within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\".");
            valid = false;
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_ADJUSTMENT_CLOCK);
    if (it == config.end()) {
        errs->push_back("\"" FLASH_PTP_JSON_CFG_ADJUSTMENT_CLOCK "\" must be specified within " \
                "items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\".");
        valid = false;
    }
    else if (!it->is_string()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_ADJUSTMENT_CLOCK "\" within " \
                "items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" must be \"") +
                Json("").type_name() + "\".");
        valid = false;
    }

    if (type == AdjustmentType::pidController) {
        if (valid)
            valid = PIDController::validateConfig(config, errs);
        else
            PIDController::validateConfig(config, errs);
    }

    return valid;
}

void Adjustment::setConfig(const Json &config)
{
    Json::const_iterator it;

    _clockID = -1;
    _clockName.clear();
    it = config.find(FLASH_PTP_JSON_CFG_ADJUSTMENT_CLOCK);
    it->get_to(_clockName);
}

bool Adjustment::prepare()
{
    if (_clockID == -1) {
        if (_clockName == FLASH_PTP_SYSTEM_CLOCK_NAME)
            _clockID = CLOCK_REALTIME;
        else
            _clockID = network::getPHCClockIDByName(_clockName);
    }

    return _clockID != -1;
}

void Adjustment::finalize(const std::vector<client::Server*> &servers)
{
    for (unsigned i = 0; i < servers.size(); ++i)
        servers[i]->calculation()->setAdjustment(false);
}

bool Adjustment::initAdj(std::vector<client::Server*> &servers)
{
    if (_clockID == -1)
        return false;

    if (servers.size() == 0)
        return false;

    for (unsigned i = 0; i < servers.size(); ++i) {
        if (!servers[i]->calculation()->hasAdjustment() ||
             servers[i]->clockID() != _clockID)
            return false;
    }

    return true;
}

}
}


