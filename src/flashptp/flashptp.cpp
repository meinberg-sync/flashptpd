/*
 * @file flashptp.cpp
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

#include <flashptp/flashptp.h>

namespace flashptp {

bool FlashPTP::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    Json::const_iterator it, iit;
    bool valid = true;

    if (errs)
        errs->clear();

    it = config.find(FLASH_PTP_JSON_CFG_LOGGING);
    if (it != config.end()) {
        if (!it->is_object()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_LOGGING "\" " \
                    "must be \"") + Json::object().type_name() + "\".");
            valid = false;
        }
        else {
            cppLog::LogType lt;
            cppLog::LogSeverity ls;
            for (auto &pit: it->items()) {
                lt = cppLog::logTypeFromStr(pit.key().c_str());
                if (lt == cppLog::LogType::invalid) {
                    errs->push_back(std::string("\"") + pit.key() + "\" is not a valid property " \
                            "within object \"" FLASH_PTP_JSON_CFG_LOGGING "\".");
                    valid = false;
                    continue;
                }

                iit = pit.value().find(CPP_LOG_CONFIG_INSTANCE_ENABLED);
                if (iit != pit.value().end() && !iit->is_boolean()) {
                    errs->push_back(std::string("Type of property \"" CPP_LOG_CONFIG_INSTANCE_ENABLED "\" " \
                            "within object \"") + pit.key() + "\" " \
                            "must be \"" + Json(false).type_name() + "\".");
                    valid = false;
                }

                iit = pit.value().find(CPP_LOG_CONFIG_INSTANCE_SEVERITY);
                if (iit != pit.value().end()) {
                    if (!iit->is_string()) {
                        errs->push_back(std::string("Type of property \"" CPP_LOG_CONFIG_INSTANCE_SEVERITY "\" " \
                            "within object \"") + pit.key() + "\" " \
                            "must be \"" + Json("").type_name() + "\".");
                        valid = false;
                    }
                    else {
                        ls = cppLog::logSeverityFromStr(iit->get<std::string>().c_str());
                        if (ls == cppLog::LogSeverity::invalid) {
                            errs->push_back(std::string("\"") + iit->get<std::string>() + "\" is not a valid value " \
                                    "for property \"" CPP_LOG_CONFIG_INSTANCE_SEVERITY "\".");
                            valid = false;
                        }
                    }
                }

                if (lt == cppLog::LogType::file) {
                    iit = pit.value().find(CPP_LOG_CONFIG_INSTANCE_FILENAME);
                    if (iit == pit.value().end()) {
                        errs->push_back(std::string("\"" CPP_LOG_CONFIG_INSTANCE_FILENAME "\" must be specified " \
                                "within object \"") + pit.key() + "\".");
                        valid = false;
                    }
                }
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE);
    if (it != config.end()) {
        if (!client::ClientMode::validateConfig(*it, errs))
            valid = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE);
    if (it != config.end()) {
        if (!server::ServerMode::validateConfig(*it, errs))
            valid = false;
    }

    return valid;
}

bool FlashPTP::setConfig(const Json &config, const std::string &configFile, std::vector<std::string> *errs)
{
    if (errs) {
        if (!validateConfig(config, errs))
            return false;
    }

    if (_running) {
        cppLog::errorf("Could not set configuration, client or server mode is currently running");
        return false;
    }

    Json::const_iterator it, iit;
    int ms = 3000;

    if (!configFile.empty())
        _configFile = configFile;

    if (config == _config)
        goto out_write;

    it = config.find(FLASH_PTP_JSON_CFG_LOGGING);
    if (it != config.end())
        cppLog::init(*it);
    else
        cppLog::exit();

    if (!network::initialized()) {
        network::init();
        while (ms > 0 && !network::initialized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ms -= 100;
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE);
    if (it != config.end())
        _clientMode.setConfig(*it);
    else
        _clientMode.setConfig(Json::object());

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE);
    if (it != config.end())
        _serverMode.setConfig(*it);
    else
        _serverMode.setConfig(Json::object());

    _config = config;

out_write:
    if (!_configFile.empty()) {
        std::ofstream ofs(_configFile);
        ofs << _config.dump(4, ' ', false, Json::error_handler_t::replace);
    }

    return true;
}

void FlashPTP::start()
{
    _running = true;
    _clientMode.start();
    _serverMode.start();
}

void FlashPTP::stop()
{
    _clientMode.stop();
    _serverMode.stop();
    _running = false;
}

}
