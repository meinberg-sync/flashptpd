/*
 * @file pidController.cpp
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

#include <flashptp/adjustment/pidController.h>
#include <flashptp/client/clientMode.h>

namespace flashptp {
namespace adjustment {

bool PIDController::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    Json::const_iterator it;
    bool valid = true;

    it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_PROPORTIONAL_RATIO);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_P_RATIO);

    if (it != config.end()) {
        if (!it->is_number()) {
            errs->push_back(std::string("Type of property \"") + it.key() + "\" within items of \"" \
                    FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" must be \"" + Json(0.0).type_name() + "\".");
            valid = false;
        }
        else {
            double kp = it->get<double>();
            if (kp < FLASH_PTP_PID_CONTROLLER_P_RATIO_MIN || kp > FLASH_PTP_PID_CONTROLLER_P_RATIO_MAX) {
                errs->push_back(std::string("Value of property \"") + it.key() + "\" within items of \"" \
                        FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" must be between " +
                        std::to_string(FLASH_PTP_PID_CONTROLLER_P_RATIO_MIN) + " and " +
                        std::to_string(FLASH_PTP_PID_CONTROLLER_P_RATIO_MAX) + ".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_INTEGRAL_RATIO);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_I_RATIO);

    if (it != config.end()) {
        if (!it->is_number()) {
            errs->push_back(std::string("Type of property \"") + it.key() + "\" within items of \"" \
                    FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" must be \"" + Json(0.0).type_name() + "\".");
            valid = false;
        }
        else {
            double ki = it->get<double>();
            if (ki < FLASH_PTP_PID_CONTROLLER_I_RATIO_MIN || ki > FLASH_PTP_PID_CONTROLLER_I_RATIO_MAX) {
                errs->push_back(std::string("Value of property \"") + it.key() + "\" within items of \"" \
                        FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" must be between " +
                        std::to_string(FLASH_PTP_PID_CONTROLLER_I_RATIO_MIN) + " and " +
                        std::to_string(FLASH_PTP_PID_CONTROLLER_I_RATIO_MAX) + ".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_DIFFERENTIAL_RATIO);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_D_RATIO);

    if (it != config.end()) {
        if (!it->is_number()) {
            errs->push_back(std::string("Type of property \"") + it.key() + "\" within items of \"" \
                    FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" must be \"" + Json(0.0).type_name() + "\".");
            valid = false;
        }
        else {
            double kd = it->get<double>();
            if (kd < FLASH_PTP_PID_CONTROLLER_D_RATIO_MIN || kd > FLASH_PTP_PID_CONTROLLER_D_RATIO_MAX) {
                errs->push_back(std::string("Value of property \"") + it.key() + "\" within items of \"" \
                        FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" must be between " +
                        std::to_string(FLASH_PTP_PID_CONTROLLER_D_RATIO_MIN) + " and " +
                        std::to_string(FLASH_PTP_PID_CONTROLLER_D_RATIO_MAX) + ".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_STEP_THRESHOLD);
    if (it != config.end()) {
        if (!it->is_number_unsigned()) {
            errs->push_back(std::string("Type of property \"") + it.key() + "\" within items of \"" \
                    FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" must be \"" + Json((unsigned)1).type_name() + "\".");
            valid = false;
        }
    }

    return valid;
}

void PIDController::setConfig(const Json &config)
{
    Adjustment::setConfig(config);

    Json::const_iterator it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_PROPORTIONAL_RATIO);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_P_RATIO);
    if (it != config.end())
        it->get_to(_kp);
    else
        _kp = FLASH_PTP_PID_CONTROLLER_P_RATIO_DEFAULT;

    it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_INTEGRAL_RATIO);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_I_RATIO);
    if (it != config.end())
        it->get_to(_ki);
    else
        _ki = FLASH_PTP_PID_CONTROLLER_I_RATIO_DEFAULT;

    it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_DIFFERENTIAL_RATIO);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_D_RATIO);
    if (it != config.end())
        it->get_to(_kd);
    else
        _kd = FLASH_PTP_PID_CONTROLLER_D_RATIO_DEFAULT;

    it = config.find(FLASH_PTP_JSON_CFG_PID_CONTROLLER_STEP_THRESHOLD);
    if (it != config.end())
        it->get_to(_stepThreshold);
    else
        _stepThreshold = FLASH_PTP_PID_CONTROLLER_STEP_THRESHOLD_DEFAULT;
}

bool PIDController::adjust(std::vector<client::Server*> &servers)
{
    if (!initAdj(servers))
        return false;

    struct timex tx;
    memset(&tx, 0, sizeof(tx));
    if (clock_adjtime(_clockID, &tx) < 0) {
        cppLog::errorf("Failed to read adjustment status of %s clock: %s (%d)",
                _clockName.c_str(), strerror(errno), errno);
        return false;
    }

    _freqAggregate = (double)tx.freq / 65536000000.0;

    /*
     * "fake integral" (partial reversion of previous adjustment)
     * Summing up _integral just for logging purpose
     */
    _integral += (_freqAddend * _ki);
    _freqAggregate -= (_freqAddend - (_freqAddend * _ki));

    unsigned i;
    _timeAddend = 0;
    for (i = 0; i < servers.size(); ++i)
        _timeAddend += servers[i]->calculation()->offset();
    _timeAddend /= (int64_t)servers.size();
    int64_t offset = _timeAddend;

    if (_stepThreshold != 0 && llabs(_timeAddend) >= _stepThreshold) {
        _freqAddend = 0;
        for (unsigned i = 0; i < servers.size(); ++i)
            _freqAddend += servers[i]->calculation()->drift();
        _freqAddend /= (double)servers.size();
        _freqAggregate += _freqAddend;
        _freqAddend = 0;
    }
    else {
        _proportional = (_kp * ((double)_timeAddend / 1000000000.0));
        _freqAddend = _proportional;

        _differential = 0;
        if (_kd != 0) {
            for (i = 0; i < servers.size(); ++i)
                _differential += servers[i]->calculation()->drift();
            _differential /= (double)servers.size();
            _differential *= _kd;
            _freqAddend += _differential;
        }

        _freqAggregate += _freqAddend;
        _timeAddend = 0;
    }

    int rc = 0;
    if (_timeAddend != 0) {
        tx.modes = ADJ_SETOFFSET | ADJ_NANO;
        tx.time.tv_sec = llabs(_timeAddend) / 1000000000ULL;
        tx.time.tv_usec = llabs(_timeAddend) % 1000000000ULL;
        if (_timeAddend < 0) {
            tx.time.tv_sec = -1 * (tx.time.tv_sec + 1);
            tx.time.tv_usec = 1000000000ULL - tx.time.tv_usec;
        }
        rc = clock_adjtime(_clockID, &tx);
    }

    if (rc >= 0) {
        tx.modes = ADJ_FREQUENCY | ADJ_NANO;
        tx.freq = (long)(_freqAggregate * 65536000000.0);
        if (llabs(tx.freq) > FLASH_PTP_ADJUSTMENT_FREQ_LIMIT) {
            if (tx.freq < 0)
                tx.freq = -FLASH_PTP_ADJUSTMENT_FREQ_LIMIT;
            else
                tx.freq = FLASH_PTP_ADJUSTMENT_FREQ_LIMIT;
        }
        rc = clock_adjtime(_clockID, &tx);
    }

    if (rc >= 0) {
        if (_timeAddend == 0) {
            cppLog::tracef("PID controller of %s clock - kp %.03f (%.012f), ki %.03f (%.012f), kd %.03f (%.012f)",
                    _clockName.c_str(), _kp, _proportional, _ki, _integral, _kd, _differential);
            cppLog::debugf("Adjusted %s clock (ADJ_FREQUENCY) by %s, successfully",
                    _clockName.c_str(), nanosecondsToStr(offset).c_str());
        }
        else
            cppLog::infof("Step Threshold (%s) exceeded - Stepped %s clock by %s, successfully",
                    nanosecondsToStr(_stepThreshold).c_str(), _clockName.c_str(), nanosecondsToStr(offset).c_str());
    }
    else
        cppLog::errorf("%s clock could not be adjusted: %s (%d)",
                _clockName.c_str(), strerror(errno), errno);

    return rc >= 0;
}

void PIDController::finalize(const std::vector<client::Server*> &servers)
{
    Adjustment::finalize(servers);
    if (_ki != 0 && _timeAddend == 0)
        return;

    for (unsigned i = 0; i < servers.size(); ++i) {
        if (servers[i]->calculation()->size() > 1)
            servers[i]->calculation()->clear();
    }
}

}
}
