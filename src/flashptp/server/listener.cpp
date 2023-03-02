/*
 * @file listener.cpp
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

#include <flashptp/server/listener.h>
#include <flashptp/server/serverMode.h>
#include <flashptp/network/network.h>

namespace flashptp {
namespace server {

Listener::Listener(ServerMode *serverMode, const Json &config)
{
    _serverMode = serverMode;
    setConfig(config);
}

bool Listener::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    if (!config.is_object()) {
        errs->push_back(std::string("Type of items within \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS "\" " \
                "must be \"") + Json::object().type_name() + "\".");
        return false;
    }

    Json::const_iterator it;
    bool valid = true;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_INTERFACE);
    if (it == config.end()) {
        errs->push_back(std::string("\"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_INTERFACE "\" must be specified " \
                "within items of \"") + FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS + "\".");
        valid = false;
    }
    else if (!it->is_string()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_INTERFACE "\" " \
                "within items of \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS "\" " \
                "must be \"") + Json("").type_name() + "\".");
        valid = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_EVENT_PORT);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_PORT);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"") + it.key() +
                    "\" within items of \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS "\" " \
                    "must be \"" + Json((int)0).type_name() + "\".");
            valid = false;
        }
        else {
            int port = it->get<int>();
            if (port < 0 || port > 65535) {
                errs->push_back(std::to_string(port) + " is not a valid value (0 <= n <= 65535) " \
                        "for property \"" + it.key() + "\".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_GENERAL_PORT);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_GENERAL_PORT
                    "\" within items of \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS "\" " \
                    "must be \"") + Json((int)0).type_name() + "\".");
            valid = false;
        }
        else {
            int port = it->get<int>();
            if (port < 0 || port > 65535) {
                errs->push_back(std::to_string(port) + " is not a valid value (0 <= n <= 65535) " \
                        "for property \"" + it.key() + "\".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_TIMESTAMP_LEVEL);
    if (it != config.end()) {
        if (!it->is_string()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_TIMESTAMP_LEVEL \
                    "\" within items of \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS "\" " \
                    "must be \"") + Json("").type_name() + "\".");
            valid = false;
        }
        else {
            PTPTimestampLevel level = ptpTimestampLevelFromShortStr(it->get<std::string>().c_str());
            if (level == PTPTimestampLevel::invalid) {
                errs->push_back(std::string("\"") + it->get<std::string>() + "\" is not a valid value (" +
                        enumClassToStr<PTPTimestampLevel>(&ptpTimestampLevelToShortStr) +
                        ") for property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_TIMESTAMP_LEVEL "\" " \
                        "within items of \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS "\".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_UTC_OFFSET);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_UTC_OFFSET "\" " \
                    "within items of \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS "\" " \
                    "must be \"") + Json((int)0).type_name() + "\".");
            valid = false;
        }
        else {
            int utcOffset;
            it->get_to(utcOffset);
            if (utcOffset < 0 || utcOffset > 65535) {
                errs->push_back("Value of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_UTC_OFFSET "\" " \
                        "within items of \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS "\" " \
                        "must be between 0 and 65535.");
                valid = false;
            }
        }
    }

    return valid;
}

bool Listener::setConfig(const Json &config)
{
    if (_running) {
        cppLog::errorf("Could not set configuration of " FLASH_PTP_SERVER_MODE_LISTENER " " \
                "%s, currently running", _interface.c_str());
        return false;
    }

    _invalid = false;
    Json::const_iterator it;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_INTERFACE);
    it->get_to(_interface);
    if (!network::hasInterface(_interface)) {
        _invalid = true;
        cppLog::warningf(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_INTERFACE \
                " %s will not be used, interface not found", _interface.c_str());
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_EVENT_PORT);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_PORT);
    if (it != config.end())
        it->get_to(_eventPort);
    else
        _eventPort = FLASH_PTP_UDP_EVENT_PORT;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_GENERAL_PORT);
    if (it != config.end())
        it->get_to(_generalPort);
    else
        _generalPort = _eventPort + 1;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_TIMESTAMP_LEVEL);
    if (it != config.end())
        _timestampLevel = ptpTimestampLevelFromShortStr(it->get<std::string>().c_str());
    else
        _timestampLevel = PTPTimestampLevel::hardware;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_UTC_OFFSET);
    if (it != config.end())
        it->get_to(_utcOffset);
    else
        _utcOffset = FLASH_PTP_DEFAULT_UTC_OFFSET;

    _threadName = std::string(FLASH_PTP_SERVER_MODE_LISTENER " on ") + _interface;

    return true;
}

void Listener::threadFunc()
{
    std::vector<network::SocketSpecs> specs;
    uint8_t buf[1024];
    int n;

    // Prepare the socket specifications to be provided to the network::recv function
    specs.emplace_back(_interface, AF_PACKET, 0, _timestampLevel);
    specs.emplace_back(_interface, AF_INET, _eventPort, _timestampLevel);
    specs.emplace_back(_interface, AF_INET, _generalPort, PTPTimestampLevel::invalid);
    specs.emplace_back(_interface, AF_INET6, _eventPort, _timestampLevel);
    specs.emplace_back(_interface, AF_INET6, _generalPort, PTPTimestampLevel::invalid);

    while (_running) {
        /*
         * Try to receive parts of incoming Sync Requests on the specified sockets for 100ms,
         * sleep for 5ms if no packet has been received.
         */
        n = network::recv(buf, sizeof(buf), specs, 100, _serverMode);
        if (n == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

}
}
