/*
 * @file filter.cpp
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

#include <flashptp/filter/filter.h>
#include <flashptp/filter/luckyPacket.h>
#include <flashptp/filter/medianOffset.h>
#include <flashptp/client/clientMode.h>

namespace flashptp {
namespace filter {

const char *Filter::typeToStr(FilterType t)
{
    switch (t) {
    case FilterType::luckyPacket: return "luckyPacket";
    case FilterType::medianOffset: return "medianOffset";
    default: return "invalid";
    }
}

FilterType Filter::typeFromStr(const char *str)
{
    for (int i = (int)FilterType::invalid + 1; i <= (int)FilterType::max; ++i) {
        if (strcasecmp(str, Filter::typeToStr((FilterType)i)) == 0)
            return (FilterType)i;
    }
    return FilterType::invalid;
}

Filter *Filter::make(const Json &config)
{
    Filter *filt = nullptr;
    FilterType type = Filter::typeFromStr(
            config.find(FLASH_PTP_JSON_CFG_FILTER_TYPE)->get<std::string>().c_str());

    switch (type) {
    case FilterType::luckyPacket: filt = new LuckyPacket(); break;
    case FilterType::medianOffset: filt = new MedianOffset(); break;
    default: break;
    }

    if (filt)
        filt->setConfig(config);

    return filt;
}

bool Filter::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    if (!config.is_object()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS "\" " \
                "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                "must be \"") + Json::object().type_name() + "\".");
        return false;
    }

    Json::const_iterator it;
    bool valid = true;
    FilterType type = FilterType::invalid;

    it = config.find(FLASH_PTP_JSON_CFG_FILTER_TYPE);
    if (it == config.end()) {
        errs->push_back("\"" FLASH_PTP_JSON_CFG_FILTER_TYPE "\" must be specified within items of \"" \
                FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS "\".");
        valid = false;
    }
    else if (!it->is_string()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_FILTER_TYPE "\" within items of \""
                FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS "\" must be \"") + Json("").type_name() + "\".");
        valid = false;
    }
    else {
        type = Filter::typeFromStr(it->get<std::string>().c_str());
        if (type == FilterType::invalid) {
            errs->push_back(std::string("\"") + it->get<std::string>() + "\" is not a valid \"" \
                    FLASH_PTP_JSON_CFG_FILTER_TYPE "\" (" + enumClassToStr<FilterType>(&Filter::typeToStr) +
                    ") within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS "\".");
            valid = false;
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_FILTER_SIZE);
    if (it != config.end()) {
        if (!it->is_number_unsigned()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_FILTER_SIZE "\" " \
                    "within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS "\" objects must be \"") +
                    Json((unsigned)1).type_name() + "\".");
            valid = false;
        }
        else if (it->get<unsigned>() == 0) {
            errs->push_back(std::to_string(it->get<unsigned>()) + " is not a valid value (0 < n) " \
                    "for property \"" FLASH_PTP_JSON_CFG_FILTER_SIZE "\".");
            valid = false;
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_FILTER_PICK);
    if (it != config.end()) {
        if (!it->is_number_unsigned()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_FILTER_PICK "\" " \
                    "within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS "\" objects must be \"") +
                    Json((unsigned)1).type_name() + "\".");
            valid = false;
        }
        else if (it->get<unsigned>() == 0) {
            errs->push_back(std::to_string(it->get<unsigned>()) + " is not a valid value (0 < n) " \
                    "for property \"" FLASH_PTP_JSON_CFG_FILTER_PICK "\".");
            valid = false;
        }
    }

    return valid;
}

void Filter::setConfig(const Json &config)
{
    Json::const_iterator it;

    it = config.find(FLASH_PTP_JSON_CFG_FILTER_SIZE);
    if (it != config.end())
        it->get_to(_size);
    else
        _size = FLASH_PTP_DEFAULT_FILTER_SIZE;

    it = config.find(FLASH_PTP_JSON_CFG_FILTER_PICK);
    if (it != config.end())
        it->get_to(_pick);
    else
        _pick = FLASH_PTP_DEFAULT_FILTER_PICK;
}

void Filter::insert(client::Sequence *seq)
{
    if (_unfiltered.size() &&
        _unfiltered.back()->timestampLevel() != seq->timestampLevel())
        clear();

    while (_unfiltered.size() >= _size) {
        delete _unfiltered.front();
        _unfiltered.pop_front();
    }

    _unfiltered.push_back(seq);
}

void Filter::clear()
{
    while (_unfiltered.size()) {
        delete _unfiltered.front();
        _unfiltered.pop_front();
    }
}

}
}


