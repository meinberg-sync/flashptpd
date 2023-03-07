/*
 * @file server.cpp
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

#include <flashptp/client/server.h>
#include <flashptp/client/clientMode.h>
#include <flashptp/network/network.h>
#include <flashptp/calculation/arithmeticMean.h>

namespace flashptp {
namespace client {

Server::Server(const Json &config)
{
    setConfig(config);
}

Server::~Server()
{
    while (_filters.size()) {
        delete _filters.back();
        _filters.pop_back();
    }

    if (_calculation)
        delete _calculation;

    std::unique_lock ul(_mutex);
    while (_sequences.size()) {
        delete _sequences.back();
        _sequences.pop_back();
    }
}

const char *Server::stateToStr(ServerState s)
{
    switch (s) {
    case ServerState::initializing: return "?";
    case ServerState::unreachable: return "!";
    case ServerState::collecting: return "^";
    case ServerState::falseticker: return "-";
    case ServerState::used: return "+";
    case ServerState::syspeer: return "*";
    default: return " ";
    }
}

const char *Server::stateToLongStr(ServerState s)
{
    switch (s) {
    case ServerState::initializing: return "Initializing";
    case ServerState::unreachable: return "Unreachable";
    case ServerState::collecting: return "Collecting";
    case ServerState::ready: return "Ready";
    case ServerState::falseticker: return "Falseticker";
    case ServerState::used: return "Used";
    case ServerState::syspeer: return "System Peer";
    default: return "Unknown";
    }
}

bool Server::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    if (!config.is_object()) {
        errs->push_back(std::string("Type of items within \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                "must be \"") + Json::object().type_name() + "\".");
        return false;
    }

    Json::const_iterator it, iit;
    int8_t requestInterval, stateInterval;
    bool valid = true;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_ADDRESS);
    if (it == config.end()) {
        errs->push_back(std::string("\"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_ADDRESS "\" must be specified " \
                "within items of \"") + FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS + "\".");
        valid = false;
    }
    else if (!it->is_string()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_ADDRESS "\" " \
                "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                "must be \"") + Json("").type_name() + "\".");
        valid = false;
    }
    else if (!network::Address::saddrFromStr(it->get<std::string>().c_str())) {
        errs->push_back(std::string("\"") + it->get<std::string>() + "\" is not a valid value " \
                "for property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_ADDRESS "\".");
        valid = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_EVENT_PORT);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_PORT);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"") + it.key() + "\" " \
                    "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
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

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_GENERAL_PORT);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_GENERAL_PORT "\" " \
                    "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
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

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_INTERFACE);
    if (it == config.end()) {
        errs->push_back(std::string("\"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_INTERFACE "\" must be specified " \
                "within items of \"") + FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS + "\".");
        valid = false;
    }
    else if (!it->is_string()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_INTERFACE "\" " \
                "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                "must be \"") + Json("").type_name() + "\".");
        valid = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_EVENT_PORT);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_PORT);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"") + it.key() + "\" " \
                    "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
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

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_GENERAL_PORT);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_GENERAL_PORT \
                    "\" within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" must be \"") +
                    Json((int)0).type_name() + "\".");
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

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_ONE_STEP);
    if (it != config.end() && !it->is_boolean()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_ONE_STEP "\" " \
                "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                "must be \"") + Json(false).type_name() + "\".");
        valid = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SYNC_TLV);
    if (it != config.end() && !it->is_boolean()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SYNC_TLV "\" " \
                "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                "must be \"") + Json(false).type_name() + "\".");
        valid = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_REQUEST_INTERVAL);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_INTERVAL);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"") + it.key() + "\" " \
                    "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                    "must be \"" + Json((int)0).type_name() + "\".");
            valid = false;
        }
        else {
            requestInterval = it->get<int8_t>();
            if (requestInterval < -7 || requestInterval > 7) {
                errs->push_back(std::to_string(requestInterval) + " is not a valid value (-7 <= n <= +7) " \
                        "for property \"" + it.key() + "\".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_STATE_INTERVAL);
    if (it != config.end()) {
        if (!it->is_number_unsigned()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_STATE_INTERVAL \
                    "\" within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                    "must be \"") + Json((int)0).type_name() + "\".");
            valid = false;
        }
        else {
            stateInterval = it->get<int8_t>();
            if (stateInterval < requestInterval || stateInterval > 7) {
                errs->push_back(std::to_string(stateInterval) + " is not a valid value (" +
                        std::to_string(requestInterval) + " <= n <= +7) for property \"" + it.key() + "\".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_MS_TIMEOUT);
    if (it != config.end()) {
        if (!it->is_number_unsigned()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_MS_TIMEOUT "\" " \
                    "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                    "must be \"") + Json((unsigned)1).type_name() + "\".");
            valid = false;
        }
        else {
            uint32_t msTimeout = it->get<uint32_t>();
            if (msTimeout < 10 || msTimeout > 10000) {
                errs->push_back(std::to_string(msTimeout) + " is not a valid value (10 <= n <= 10000) " \
                        "for property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_MS_TIMEOUT "\".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_SERVER_TIMESTAMP_LEVEL);
    if (it != config.end()) {
        if (!it->is_string()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_SERVER_TIMESTAMP_LEVEL \
                    "\" within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" " \
                    "must be \"") + Json("").type_name() + "\".");
            valid = false;
        }
        else {
            PTPTimestampLevel level = ptpTimestampLevelFromShortStr(it->get<std::string>().c_str());
            if (level == PTPTimestampLevel::invalid) {
                errs->push_back(std::string("\"") + it->get<std::string>() + "\" is not a valid value (" +
                        enumClassToStr<PTPTimestampLevel>(&ptpTimestampLevelToShortStr) + ") " \
                        "for property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_SERVER_TIMESTAMP_LEVEL "\" " \
                        "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS);
    if (it != config.end()) {
        if (!it->is_array()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS "\" " \
                    "within items of \"" FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS "\" must be \"") +
                    Json::array().type_name() + "\".");
            valid = false;
        }
        else {
            for (const auto &f: *it) {
                if (valid)
                    valid = filter::Filter::validateConfig(f, errs);
                else
                    filter::Filter::validateConfig(f, errs);
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_CALCULATION);
    if (it != config.end()) {
        if (valid)
            valid = calculation::Calculation::validateConfig(*it, errs);
        else
            calculation::Calculation::validateConfig(*it, errs);
    }

    return valid;
}

bool Server::setConfig(const Json &config)
{
    if (_running) {
        cppLog::errorf("Could not set configuration of " FLASH_PTP_CLIENT_MODE_SERVER " " \
                "%s, currently running", _dstAddress.str().c_str());
        return false;
    }

    _invalid = false;
    Json::const_iterator it;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_ADDRESS);
    _dstAddress = network::Address(it->get<std::string>());
    _threadName = std::string(FLASH_PTP_CLIENT_MODE_SERVER " ") + _dstAddress.str();

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_EVENT_PORT);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_PORT);
    if (it != config.end())
        it->get_to(_dstEventPort);
    else
        _dstEventPort = FLASH_PTP_UDP_EVENT_PORT;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_GENERAL_PORT);
    if (it != config.end())
        it->get_to(_dstGeneralPort);
    else
        _dstGeneralPort = _dstEventPort + 1;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_INTERFACE);
    it->get_to(_srcInterface);
    if (!network::hasInterface(_srcInterface)) {
        _invalid = true;
        cppLog::warningf(FLASH_PTP_CLIENT_MODE_SERVER " %s will not be used, source interface %s not found",
                _dstAddress.str().c_str(), _srcInterface.c_str());
    }
    else if (!network::getFamilyAddress(_srcInterface, _dstAddress.family())) {
        _invalid = true;
        cppLog::warningf(FLASH_PTP_CLIENT_MODE_SERVER " %s will not be used, no %s address found on " \
                "source interface %s", _dstAddress.str().c_str(),
                network::Address::familyToStr(_dstAddress.family()), _srcInterface.c_str());
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_EVENT_PORT);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_PORT);
    if (it != config.end())
        it->get_to(_srcEventPort);
    else
        _srcEventPort = FLASH_PTP_UDP_EVENT_PORT;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_GENERAL_PORT);
    if (it != config.end())
        it->get_to(_srcGeneralPort);
    else
        _srcGeneralPort = _srcEventPort + 1;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_ONE_STEP);
    if (it != config.end())
        it->get_to(_oneStep);
    else
        _oneStep = false;

    if (_oneStep) {
        cppLog::warningf("flashptpd can only provide %s Timestamps in One-Step mode " \
                "(" FLASH_PTP_CLIENT_MODE_SERVER " %s)", ptpTimestampLevelToStr(PTPTimestampLevel::user),
                _dstAddress.str().c_str());
        _syncTLV = true;
    }
    else {
        it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SYNC_TLV);
        if (it != config.end())
            it->get_to(_syncTLV);
        else
            _syncTLV = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_REQUEST_INTERVAL);
    if (it == config.end())
        it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_INTERVAL);
    if (it != config.end())
        it->get_to(_interval);
    else
        _interval = FLASH_PTP_DEFAULT_INTERVAL;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_STATE_INTERVAL);
    if (it != config.end())
        it->get_to(_stateInterval);
    else
        _stateInterval = FLASH_PTP_DEFAULT_STATE_INTERVAL;

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_MS_TIMEOUT);
    if (it != config.end())
        it->get_to(_msTimeout);
    else
        _msTimeout = FLASH_PTP_DEFAULT_TIMEOUT_MS;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_SERVER_TIMESTAMP_LEVEL);
    if (it != config.end())
        _timestampLevel = ptpTimestampLevelFromShortStr(it->get<std::string>().c_str());
    else
        _timestampLevel = PTPTimestampLevel::hardware;

    while (_filters.size()) {
        delete _filters.back();
        _filters.pop_back();
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS);
    if (it != config.end()) {
        for (const auto &f: *it)
            _filters.push_back(filter::Filter::make(f));
    }

    if (_calculation) {
        delete _calculation;
        _calculation = nullptr;
    }

    it = config.find(FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_CALCULATION);
    if (it != config.end())
        _calculation = calculation::Calculation::make(*it);
    else
        _calculation = new calculation::ArithmeticMean();

    return true;
}

std::vector<network::SocketSpecs> Server::specs() const
{
    std::vector<network::SocketSpecs> specs;
    switch (_dstAddress.family()) {
    case AF_PACKET: {
        specs.emplace_back(_srcInterface, _dstAddress.family(), 0, _timestampLevel);
        break;
    }
    case AF_INET:
    case AF_INET6: {
        specs.emplace_back(_srcInterface, _dstAddress.family(), _srcEventPort, _timestampLevel);
        specs.emplace_back(_srcInterface, _dstAddress.family(), _srcGeneralPort, PTPTimestampLevel::invalid);
        break;
    }
    default: break;
    }
    return specs;
}

std::string Server::clockName() const
{
    PTPTimestampLevel timestampLevel = _calculation->timestampLevel();
    if (timestampLevel == PTPTimestampLevel::invalid)
        return "-";
    else if (timestampLevel <= PTPTimestampLevel::socket)
        return FLASH_PTP_SYSTEM_CLOCK_NAME;
    else {
        std::shared_lock lock(_mutex);
        return _clockName;
    }
}

clockid_t Server::clockID() const
{
    PTPTimestampLevel timestampLevel = _calculation->timestampLevel();
    if (timestampLevel == PTPTimestampLevel::invalid)
        return -1;
    else if (timestampLevel <= PTPTimestampLevel::socket)
        return CLOCK_REALTIME;
    else {
        std::shared_lock lock(_mutex);
        return _clockID;
    }
}

Sequence *Server::processMessage(PTP2Message *msg, FlashPTPRespTLV *tlv, PTPTimestampLevel timestampLevel,
        const struct timespec *timestamp)
{
    Sequence *seq;
    std::unique_lock ul(_mutex);
    for (unsigned i = 0; i < _sequences.size(); ++i) {
        seq = _sequences[i];
        if (seq->sequenceID() != msg->seqID)
            continue;

        if (seq->timedOut()) {
            _sequences.erase(_sequences.begin() + i);
            onSequenceTimeout(seq);
            break;
        }

        if (msg->msgType == (uint8_t)PTPMessageType::sync) {
            if (seq->hasT4())
                break;

            cppLog::tracef("Received %s Response (seq id %u, %s timestamp) from %s",
                    ptpMessageTypeToStr(PTPMessageType::sync), msg->seqID,
                    ptpTimestampLevelToShortStr(timestampLevel), _dstAddress.str().c_str());
        }
        else if (msg->msgType == (uint8_t)PTPMessageType::followUp) {
            if (seq->hasT3())
                break;

            cppLog::tracef("Received %s Response (seq id %u) from %s",
                    ptpMessageTypeToStr(PTPMessageType::followUp), msg->seqID, _dstAddress.str().c_str());
        }
        else
            break;

        seq->merge(msg, tlv, timestampLevel, timestamp);
        if (seq->complete()) {
            _sequences.erase(_sequences.begin() + i);
            seq->finish();
            onSequenceComplete(seq);
        }
        else
            break;
    }

    return nullptr;
}

void Server::calcStdDev()
{
    double mean = 0, var = 0;
    unsigned i, cnt;
    for (i = 0, cnt = 0; i < FLASH_PTP_CLIENT_MODE_SERVER_OFFSET_HISTORY_SIZE; ++i) {
        if (_stdDevHistory[i] != INT64_MAX) {
            mean += _stdDevHistory[i];
            ++cnt;
        }
    }
    mean /= (double)cnt;

    if (cnt <= 1) {
        _stdDev = INT64_MAX;
        return;
    }

    for (i = 0; i < FLASH_PTP_CLIENT_MODE_SERVER_OFFSET_HISTORY_SIZE; ++i) {
        if (_stdDevHistory[i] != INT64_MAX)
            var += pow((double)_stdDevHistory[i] - mean, 2);
    }
    var /= (double)(cnt - 1);

    _stdDev = sqrt(var);
}

void Server::onSequenceComplete(Sequence *seq)
{
    // This function must be called with exclusively locked _mutex, see processMessage
    _reach <<= 1;
    _reach |= 1;

    if (seq->serverStateDSRequested()) {
        _serverStateDSValid = seq->serverStateDSValid();
        if (_serverStateDSValid)
            _serverStateDS = seq->serverStateDS();
    }

    cppLog::tracef("Request Sequence complete - Server %s, ID %u, Reach 0x%04x, Delay %s, Offset %s",
            _dstAddress.str().c_str(), seq->sequenceID(), _reach & 0xffff,
            nanosecondsToStr(seq->meanPathDelay()).c_str(), nanosecondsToStr(seq->offset()).c_str());

    std::deque<Sequence*> seqs{ seq }, nextseqs;
    for (auto filt: _filters) {
        while (seqs.size() > 0) {
            filt->insert(seqs.front());
            seqs.pop_front();
            if (filt->full())
                filt->filter(nextseqs);
        }
        seqs.swap(nextseqs);
    }

    if (!seqs.size())
        return;

    while (seqs.size()) {
        _stdDevHistory[_stdDevIndex] = seqs.front()->offset();
        if (++_stdDevIndex == FLASH_PTP_CLIENT_MODE_SERVER_OFFSET_HISTORY_SIZE)
            _stdDevIndex = 0;
        _calculation->insert(seqs.front());
        seqs.pop_front();
    }

    calcStdDev();

    _calculation->calculate();
    if (_calculation->fullyLoaded()) {
        if (_state < ServerState::ready)
            _state = ServerState::ready;

        cppLog::debugf("Calculation complete - Server %s, Delay %s, Offset %s, Drift %s",
                _dstAddress.str().c_str(),
                nanosecondsToStr(_calculation->delay()).c_str(),
                nanosecondsToStr(_calculation->offset()).c_str(),
                nanosecondsToStr(_calculation->drift() * 1000000000.0).c_str());
    }
    else if (_state < ServerState::collecting)
        _state = ServerState::collecting;
}

void Server::onSequenceTimeout(Sequence *seq)
{
    // This function must be called with exclusively locked _mutex, see processMessage
    _reach <<= 1;
    _reach &= ~1;

    if (seq->serverStateDSRequested())
        _serverStateDSValid = false;

    if (_reach == 0xfffe)
        cppLog::infof("Request timed out unexpectedly (Reach was 0xffff) - Server %s, ID %u",
                _dstAddress.str().c_str(), seq->sequenceID());
    else
        cppLog::debugf("Request timed out - Server %s, ID %u, Reach 0x%04x",
                _dstAddress.str().c_str(), seq->sequenceID(), _reach & 0xffff);

    if (_reach == 0) {
        if (_state > ServerState::unreachable)
            cppLog::warningf("Server %s is not reachable any longer (Reach 0x%04x)",
                    _dstAddress.str().c_str(), _reach & 0xffff);
        _state = ServerState::unreachable;
        _calculation->reset();
        _serverStateDSValid = false;
    }

    bool remove = true;
    if (_filters.size()) {
        if (!(_reach & 0xf)) {
            // clear filters if last four (0xf) sequences have timed out
            // if filters have been cleared already, remove calculation entry
            for (auto filt: _filters) {
                if (!filt->empty()) {
                    filt->clear();
                    remove = false;
                }
            }
        }
    }

    if (remove)
        _calculation->remove();

    _stdDevHistory[_stdDevIndex] = INT64_MAX;
    if (++_stdDevIndex == FLASH_PTP_CLIENT_MODE_SERVER_OFFSET_HISTORY_SIZE)
        _stdDevIndex = 0;
    calcStdDev();

    delete seq;
}

std::string Server::printState() const
{
    std::stringstream sstr, tsstr;
    std::string clockStr = clockName();

    std::shared_lock sl(_mutex);
    sstr << std::setfill(' ');
    sstr << Server::stateToStr(_state) << " ";
    sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_SERVER) << std::left << _dstAddress.str();
    sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_CLOCK) << std::left << clockStr;

    if (_serverStateDSValid) {
        sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_BTCA) << std::left <<
                _serverStateDS.toBTCAStr();
    }
    else
        sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_BTCA) << std::left << "unknown";

    tsstr << "0x";
    tsstr << std::setfill('0') << std::setw(4) << std::hex << (unsigned)(_reach & 0xffff);
    sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_REACH) << std::left << tsstr.str();
    tsstr.str("");

    sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_INTV) << std::left << (int)_interval;

    if (_calculation->valid()) {
        sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_DELAY) << std::left <<
                nanosecondsToStr(_calculation->delay());
        sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_OFFSET) << std::left <<
                nanosecondsToStr(_calculation->offset());
    }
    else {
        sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_DELAY) << std::left << "-";
        sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_OFFSET) << std::left << "-";
    }

    if (_stdDev == INT64_MAX)
        sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_STD_DEV) << std::left << "-";
    else
        sstr << std::setw(FLASH_PTP_CLIENT_MODE_SERVER_STATS_COL_STD_DEV) << std::left <<
                nanosecondsToStr(_stdDev);

    return sstr.str();
}

void Server::resetState()
{
    _calculation->reset();

    std::unique_lock ul(_mutex);
    _state = ServerState::initializing;
    _reach = 0;

    _serverStateDSValid = false;

    _clockName.clear();
    _clockID = -1;

    while (_sequences.size()) {
        delete _sequences.back();
        _sequences.pop_back();
    }

    for (unsigned i = 0; i < FLASH_PTP_CLIENT_MODE_SERVER_OFFSET_HISTORY_SIZE; ++i)
        _stdDevHistory[i] = INT64_MAX;
    _stdDevIndex = 0;
    _stdDev = INT64_MAX;
}

void Server::checkSequenceTimeouts()
{
    Sequence *seq;
    std::unique_lock ul(_mutex);
    for (unsigned i = 0; i < _sequences.size(); ++i) {
        seq = _sequences[i];
        if (!seq->timedOut())
            continue;

        _sequences.erase(_sequences.begin() + i);
        onSequenceTimeout(seq);
        --i;
    }
}

void Server::threadFunc()
{
    uint8_t buf[1024];
    PTP2Message *ptp;
    FlashPTPReqTLV tlv;

    bool requestServerStateDS;
    PTPTimestampLevel currentLevel;
    struct timespec timestamp;
    uint16_t sequenceID;
    std::string phcName;
    clockid_t phcID;
    int32_t usec, stateUsec;
    time_t tprev;


    ptp = (PTP2Message*)buf;
    sequenceID = 0;

    resetState();
    // Try to find out the PHC name and id if hardware timestamping is to be used for this server
    if (_timestampLevel == PTPTimestampLevel::hardware &&
        network::getInterfaceTimestampLevel(_srcInterface) == PTPTimestampLevel::hardware &&
        network::getInterfacePHCInfo(_srcInterface, &phcName, &phcID))
        setClock(phcName, phcID);

    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    tprev = timestamp.tv_sec;
    stateUsec = 0;
    usec = 0;

    while (_running) {
        // Check for timed out sequences once per second
        clock_gettime(CLOCK_MONOTONIC, &timestamp);
        if (timestamp.tv_sec != tprev) {
            tprev = timestamp.tv_sec;
            checkSequenceTimeouts();
        }

        if (usec)
            goto interval_sleep;

        // Set usec to the configured request interval (microseconds)
        usec = (int32_t)(pow(2, _interval) * 1000000L);
        // Set currentLevel to the timestamp level to be used (as input value for network::send)
        currentLevel = _timestampLevel;

        // Check, if FlashPTPServerStateDS is to be requested for the next sequence
        requestServerStateDS = _stateInterval != 0x7f && stateUsec == 0;
        tlv.txPrepare(&buf[sizeof(*ptp)], sizeof(buf) - sizeof(*ptp),
                requestServerStateDS ? FLASH_PTP_FLAG_SERVER_STATE_DS : 0);

        *ptp = PTP2Message(PTPMessageType::sync,
                _syncTLV ? (sizeof(*ptp) + tlv.len()) : sizeof(*ptp), !_oneStep);
        ptp->seqID = sequenceID;
        ptp->logMsgPeriod = _interval;

        ptp->reorder();
        if (_syncTLV)
            tlv.reorder();

        /*
         * Send a Sync Message (+ Request TLV if it shall be attached to the Sync)
         * If using one-step, add the new request sequence to the store. Otherwise, send a
         * Follow Up Message (+ Request TLV if it shall be attached to the Follow Up) and
         * add the new request sequence to the store, now.
         */
        if (network::send(ptp, ntohs(ptp->totalLen), _srcInterface, _srcEventPort, _dstAddress, _dstEventPort,
                &currentLevel, &timestamp)) {
            if (_oneStep)
                addSequence(new Sequence(_srcInterface, _srcEventPort, _srcEventPort,
                        _dstAddress, _msTimeout, sequenceID, currentLevel, &timestamp, requestServerStateDS));
            else {
                *ptp = PTP2Message(PTPMessageType::followUp,
                        _syncTLV ? sizeof(*ptp) : (sizeof(*ptp) + tlv.len()), false);
                ptp->seqID = sequenceID;
                ptp->logMsgPeriod = _interval;
                ptp->flags.timescale = (uint8_t)(currentLevel == PTPTimestampLevel::hardware);

                ptp->reorder();
                if (!_syncTLV)
                    tlv.reorder();

                if (network::send(ptp, ntohs(ptp->totalLen), _srcInterface, _srcGeneralPort,
                        _dstAddress, _dstGeneralPort))
                    addSequence(new Sequence(_srcInterface, _srcEventPort, _srcEventPort,
                            _dstAddress, _msTimeout, sequenceID, currentLevel, &timestamp, requestServerStateDS));
            }
        }

        // Reset stateUsec to the configured state interval (microseconds),
        // if a state request has been transmitted
        if (requestServerStateDS)
            stateUsec = (int32_t)(pow(2, _stateInterval) * 1000000L);

interval_sleep:
        // Sleep for the duration of the configured request interval before sending the next Sync Request.
        if (usec) {
            if (usec > 100000) {
                std::this_thread::sleep_for(std::chrono::microseconds(100000));
                stateUsec -= 100000;
                usec -= 100000;
                continue;
            }
            else {
                std::this_thread::sleep_for(std::chrono::microseconds(usec));
                stateUsec -= usec;
                usec = 0;
            }
        }

        ++sequenceID;
    }

    resetState();
}

}
}
