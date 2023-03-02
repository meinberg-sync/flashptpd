/*
 * @file calculation.cpp
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

#include <flashptp/calculation/calculation.h>
#include <flashptp/calculation/passThrough.h>
#include <flashptp/calculation/arithmeticMean.h>
#include <flashptp/client/clientMode.h>

namespace flashptp {
namespace calculation {

const char *Calculation::typeToStr(CalculationType t)
{
    switch (t) {
    case CalculationType::passThrough: return "passThrough";
    case CalculationType::arithmeticMean: return "arithmeticMean";
    default: return "invalid";
    }
}

CalculationType Calculation::typeFromStr(const char *str)
{
    for (int i = (int)CalculationType::invalid + 1; i <= (int)CalculationType::max; ++i) {
        if (strcasecmp(str, Calculation::typeToStr((CalculationType)i)) == 0)
            return (CalculationType)i;
    }
    return CalculationType::invalid;
}

Calculation::~Calculation()
{
    clear();
}

Calculation *Calculation::make(const Json &config)
{
    Calculation *calc = nullptr;
    CalculationType type = Calculation::typeFromStr(
            config.find(FLASH_PTP_JSON_CFG_CALCULATION_TYPE)->get<std::string>().c_str());

    switch (type) {
    case CalculationType::passThrough: calc = new PassThrough(); break;
    case CalculationType::arithmeticMean: calc = new ArithmeticMean(); break;
    default: break;
    }

    if (calc)
        calc->setConfig(config);

    return calc;
}

bool Calculation::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    if (!config.is_object()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_CALCULATION "\" " \
                "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                "must be \"") + Json::object().type_name() + "\".");
        return false;
    }

    bool valid = true;
    Json::const_iterator it;
    CalculationType type = CalculationType::invalid;

    it = config.find(FLASH_PTP_JSON_CFG_CALCULATION_TYPE);
    if (it == config.end()) {
        errs->push_back("\"" FLASH_PTP_JSON_CFG_CALCULATION_TYPE "\" must be specified within \"" \
                FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_CALCULATION "\" objects.");
        valid = false;
    }
    else if (!it->is_string()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CALCULATION_TYPE "\" within \""
                FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_CALCULATION "\" objects must be \"") +
                Json("").type_name() + "\".");
        valid = false;
    }
    else {
        type = Calculation::typeFromStr(it->get<std::string>().c_str());
        if (type == CalculationType::invalid) {
            errs->push_back(std::string("\"") + it->get<std::string>() + "\" is not a valid \"" \
                    FLASH_PTP_JSON_CFG_CALCULATION_TYPE "\" (" +
                    enumClassToStr<CalculationType>(&Calculation::typeToStr) +
                    ") within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_CALCULATION "\" objects.");
            valid = false;
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_CALCULATION_SIZE);
    if (it != config.end()) {
        if (!it->is_number_unsigned()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CALCULATION_SIZE "\" " \
                    "within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_CALCULATION "\" objects must be \"") +
                    Json((unsigned)1).type_name() + "\".");
            valid = false;
        }
        else if (it->get<unsigned>() < 2) {
            errs->push_back(std::to_string(it->get<unsigned>()) + " is not a valid value (2 <= n) " \
                    "for property \"" FLASH_PTP_JSON_CFG_CALCULATION_SIZE "\".");
            valid = false;
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_CALCULATION_COMPENSATION_VALUE);
    if (it != config.end()) {
        if (!it->is_number()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CALCULATION_COMPENSATION_VALUE "\" " \
                    "within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_CALCULATION "\" objects must be \"") +
                    Json((int)0).type_name() + "\".");
            valid = false;
        }
    }

    return valid;
}

void Calculation::setConfig(const Json &config)
{
    Json::const_iterator it;

    it = config.find(FLASH_PTP_JSON_CFG_CALCULATION_SIZE);
    if (it != config.end())
        it->get_to(_size);
    else if (_size == 0)
        _size = FLASH_PTP_DEFAULT_CALCULATION_SIZE;

    it = config.find(FLASH_PTP_JSON_CFG_CALCULATION_COMPENSATION_VALUE);
    if (it != config.end())
        it->get_to(_compensationValue);
    else
        _compensationValue = 0;
}

void Calculation::insert(client::Sequence *seq)
{
    std::unique_lock ul(_mutex);
    if (_sequences.size() > 0 &&
        _sequences.back()->timestampLevel() != seq->timestampLevel())
        clearLocked();

    if (_sequences.size() > 0) {
        _prevSeqValid = true;
        _prevSeqTimestamp = _sequences.back()->t1();
        _prevSeqOffset = _sequences.back()->offset();
    }

    while (_sequences.size() >= _size) {
        delete _sequences[0];
        _sequences.erase(_sequences.begin());
    }

    _sequences.push_back(seq);
    _timestampLevel = seq->timestampLevel();
}

void Calculation::clear()
{
    std::unique_lock ul(_mutex);
    clearLocked();
}

void Calculation::clearLocked()
{
    // This function must be called with exclusively locked _mutex
    _prevSeqValid = false;
    while (_sequences.size()) {
        delete _sequences.back();
        _sequences.pop_back();
    }
}

void Calculation::remove()
{
    _mutex.lock();
    _prevSeqValid = false;
    if (_sequences.size() > 0) {
        delete _sequences[0];
        _sequences.erase(_sequences.begin());
    }

    bool needsReset = _sequences.size() == 0;
    _mutex.unlock();

    if (needsReset)
        reset();
}

int64_t Calculation::windowDuration() const
{
    std::shared_lock sl(_mutex);
    if (_sequences.size() == 0)
        return 0;
    else if (_sequences.size() == 1)
        return sampleRate() * 1000000000.0;
    else
        return _sequences.back()->t1() - _sequences.front()->t1();
}

double Calculation::sampleRate() const
{
    std::shared_lock sl(_mutex);
    if (_sequences.size() > 0 && _prevSeqValid)
        return (double)(_sequences.back()->t1() - _prevSeqTimestamp) / 1000000000.0;
    else
        return 0;
}

void Calculation::reset()
{
    std::unique_lock ul(_mutex);

    clearLocked();
    _timestampLevel = PTPTimestampLevel::invalid;

    _valid = false;
    _delay = 0;
    _offset = 0;
    _drift = 0;

    _adjustment = false;
}

}
}
