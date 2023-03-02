/*
 * @file clientMode.cpp
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

#include <flashptp/client/clientMode.h>
#include <flashptp/selection/stdDev.h>
#include <flashptp/adjustment/adjtimex.h>
#include <flashptp/flashptp.h>

namespace flashptp {
namespace client {

bool ClientMode::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    if (!config.is_object()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE "\" " \
                "must be \"") + Json::object().type_name() + "\".");
        return false;
    }

    Json::const_iterator it;
    bool valid = true;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_ENABLED);
    if (it != config.end() && !it->is_boolean()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_ENABLED "\" " \
                "within object \"" FLASH_PTP_JSON_CFG_CLIENT_MODE "\" " \
                "must be \"") + Json(false).type_name() + "\".");
        valid = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS);
    if (it != config.end()) {
        if (!it->is_array()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                    "within object \"" FLASH_PTP_JSON_CFG_CLIENT_MODE "\" " \
                    "must be \"") + Json::array().type_name() + "\".");
            valid = false;
        }
        else {
            for (const auto &s: *it) {
                if (valid)
                    valid = Server::validateConfig(s, errs);
                else
                    Server::validateConfig(s, errs);
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION);
    if (it != config.end()) {
        if (valid)
            valid = selection::Selection::validateConfig(*it, errs);
        else
            selection::Selection::validateConfig(*it, errs);
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS);
    if (it != config.end()) {
        if (!it->is_array()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS "\" " \
                    "within object \"" FLASH_PTP_JSON_CFG_CLIENT_MODE "\" " \
                    "must be \"") + Json::array().type_name() + "\".");
            valid = false;
        }
        else {
            for (const auto &a: *it) {
                if (valid)
                    valid = adjustment::Adjustment::validateConfig(a, errs);
                else
                    adjustment::Adjustment::validateConfig(a, errs);
            }
        }
    }

    return valid;
}

bool ClientMode::setConfig(const Json &config, std::vector<std::string> *errs)
{
    if (errs) {
        if (!validateConfig(config, errs))
            return false;
    }

    if (_running) {
        cppLog::errorf("Could not set configuration of %s, currently running", threadName());
        return false;
    }

    cppLog::debugf("Setting configuration of %s", threadName());

    Json::const_iterator it;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_ENABLED);
    if (it != config.end())
        _enabled = it->get<bool>();
    else
        _enabled = false;

    clearServers();
    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS);
    if (it != config.end()) {
        for (const auto &s: *it)
            _servers.push_back(new Server(s));
    }

    if (_selection) {
        delete _selection;
        _selection = nullptr;
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SELECTION);
    if (it != config.end())
        _selection = selection::Selection::make(*it);
    else
        _selection = new selection::StdDev();

    while (_adjustments.size()) {
        delete _adjustments.back();
        _adjustments.pop_back();
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS);
    if (it != config.end()) {
        for (const auto &a: *it)
            _adjustments.push_back(adjustment::Adjustment::make(a));
    }
    else
        _adjustments.push_back(new adjustment::Adjtimex());

    if (_enabled) {
        if (_servers.empty())
            cppLog::warningf("%s is enabled, but no " FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS \
                    " have been configured", threadName());
        else
            cppLog::infof("%s is enabled, %d " FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS " configured",
                    threadName(), _servers.size());
    }
    else
        cppLog::infof("%s is disabled", threadName());

    return true;
}

void ClientMode::resetUnusedServersStates()
{
    for (unsigned i = 0; i < _servers.size(); ++i) {
        if (_servers[i]->state() > ServerState::ready &&
            !hasAdjustment(_servers[i]->clockID()))
            _servers[i]->setState(ServerState::ready);
    }
}

void ClientMode::printServers()
{
    if (_serversFile.empty())
        return;

    std::ofstream ofs;
    ofs.open(_serversFile.c_str());
    if (!ofs.good())
        return;

    ofs << std::setfill(' ');
    ofs << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_STATE) << std::left << "";
    ofs << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_SERVER) << std::left << "server";
    ofs << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_CLOCK) << std::left << "clock";
    ofs << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_BMCA_DS) << std::left << "p1/cc/ca/cv/p2/sr";
    ofs << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_REACH) << std::left << "reach";
    ofs << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_INTV) << std::left << "intv";
    ofs << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_DELAY) << std::left << "delay";
    ofs << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_OFFSET) << std::left << "offset";
    ofs << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_STD_DEV) << std::left << "stdDev";
    ofs << std::endl;

    for (unsigned i = 0; i < FLASH_PTP_CLIENT_MODE_SERVER_STATS_LEN; ++i)
        ofs << "=";
    ofs << std::endl;

    for (const auto s: _servers)
        ofs << s->printState() << std::endl;

    ofs.close();
}

bool ClientMode::hasAdjustment(clockid_t id) const
{
    for (const auto adj: _adjustments) {
        if (adj->clockID() == id)
            return true;
    }
    return false;
}

void ClientMode::performAdjustments()
{
    std::vector<Server*> servers;
    for (unsigned i = 0; i < _adjustments.size(); ++i) {
        if (!_adjustments[i]->prepare())
            continue;

        servers = _selection->select(_servers, _adjustments[i]->clockID());
        if (_adjustments[i]->adjust(servers))
            _adjustments[i]->finalize(servers);
    }
}

void ClientMode::onMsgReceived(PTP2Message *msg, int len, const struct sockaddr_storage *srcSockaddr,
        const struct sockaddr_storage *dstSockaddr, PTPTimestampLevel timestampLevel,
        const struct timespec *timestamp)
{
    if (!_enabled || !_running)
        return;

    if (msg->logMsgPeriod != 0x7f) {
        // This is a message belonging to a request sequence, process it to server mode (if enabled)
        _flashPTP->onReqReceived(msg, len, srcSockaddr, dstSockaddr, timestampLevel, timestamp);
        return;
    }

    FlashPTPRespTLV tlv;
    bool isRequest;
    if (FlashPTPTLVHdr::validate(msg, len, &isRequest)) {
        if (isRequest) {
            // This is a message belonging to a request sequence, process it to server mode (if enabled)
            _flashPTP->onReqReceived(msg, len, srcSockaddr, dstSockaddr, timestampLevel, timestamp);
            return;
        }

        tlv.rxRestore((uint8_t*)msg + sizeof(*msg), len - sizeof(*msg));
        if (!tlv.valid)
            return;
        tlv.reorder(true);
    }

    msg->reorder(true);

    for (auto s: _servers) {
        if (!s->dstAddress().equals(srcSockaddr))
            continue;

        s->processMessage(msg, &tlv, timestampLevel, timestamp);
        break;
    }
}

void ClientMode::threadFunc()
{
    uint8_t buf[1024];
    std::vector<network::SocketSpecs> sspecs, specs;
    struct sockaddr_storage srcSockaddr, dstSockaddr;
    PTPTimestampLevel timestampLevel;
    struct timespec timestamp;
    time_t tprev;
    int n;

    /*
     * Start all server worker threads and determine the socket specifications
     * to be used for receipt of incoming Sync Responses.
     */
    for (auto s: _servers) {
        if (!s->invalid()) {
            if (!s->start())
                continue;

            sspecs = s->specs();
            specs.insert(specs.end(), sspecs.begin(), sspecs.end());
        }
    }

    tprev = 0;
    while (_running) {
        performAdjustments();

        // Reset unused servers states and print servers file (if set) once per second
        clock_gettime(CLOCK_MONOTONIC, &timestamp);
        if (timestamp.tv_sec != tprev) {
            tprev = timestamp.tv_sec;
            resetUnusedServersStates();
            printServers();
        }

        /*
         * Try to receive Sync Responses on the specified sockets for 100ms,
         * sleep for 5ms if no packet has been received.
         */
        n = network::recv(buf, sizeof(buf), specs, 100, this);
        if (n == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Stop all server worker threads before exiting client mode worker thread.
    for (auto s: _servers) {
        if (!s->invalid())
            s->stop();
    }
}

}
}
