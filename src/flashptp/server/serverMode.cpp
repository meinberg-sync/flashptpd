/*
 * @file serverMode.cpp
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

#include <flashptp/server/serverMode.h>
#include <flashptp/flashptp.h>

namespace flashptp {
namespace server {

bool ServerMode::validateConfig(const Json &config, std::vector<std::string> *errs)
{
    if (!config.is_object()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                "must be \"") + Json::object().type_name() + "\".");
        return false;
    }

    Json::const_iterator it;
    bool valid = true;
    std::string str;
    int d;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_ENABLED);
    if (it != config.end() && !it->is_boolean()) {
        errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_ENABLED "\" " \
                "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                "must be \"") + Json(false).type_name() + "\".");
        valid = false;
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_1);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_1 "\" " \
                    "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                    "must be \"") + Json((int)0).type_name() + "\".");
            valid = false;
        }
        else {
            it->get_to(d);
            if (d < 0 || d > 255) {
                errs->push_back("Value of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_1 "\" " \
                        "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                        "must be between 0 and 255.");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_CLASS);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_CLASS "\" " \
                    "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                    "must be \"") + Json((int)0).type_name() + "\".");
            valid = false;
        }
        else {
            it->get_to(d);
            if (d < 0 || d > 255) {
                errs->push_back("Value of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_CLASS "\" " \
                        "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                        "must be between 0 and 255.");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_ACCURACY);
    if (it != config.end()) {
        if (!it->is_string()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_ACCURACY "\" " \
                    "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                    "must be \"") + Json("").type_name() + "\" (hex).");
            valid = false;
        }
        else {
            it->get_to(str);
            if (str.find("0x") == 0)
                str.erase(0, 2);
            d = strtol(str.c_str(), nullptr, 16);
            if (d < 0x17 || d > 0x31) {
                errs->push_back("Value of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_ACCURACY "\" " \
                        "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                        "must be between \"0x17\" and \"0x31\".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_VARIANCE);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_VARIANCE "\" " \
                    "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                    "must be \"") + Json((int)0).type_name() + "\".");
            valid = false;
        }
        else {
            it->get_to(d);
            if (d < 0 || d > 65535) {
                errs->push_back("Value of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_VARIANCE "\" " \
                        "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                        "must be between 0 and 65535.");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_2);
    if (it != config.end()) {
        if (!it->is_number_integer()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_2 "\" " \
                    "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                    "must be \"") + Json((int)0).type_name() + "\".");
            valid = false;
        }
        else {
            it->get_to(d);
            if (d < 0 || d > 255) {
                errs->push_back("Value of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_2 "\" " \
                        "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                        "must be between 0 and 255.");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_TIME_SOURCE);
    if (it != config.end()) {
        if (!it->is_string()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_TIME_SOURCE "\" " \
                    "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                    "must be \"") + Json("").type_name() + "\" (hex).");
            valid = false;
        }
        else {
            it->get_to(str);
            if (str.find("0x") == 0)
                str.erase(0, 2);
            d = strtol(str.c_str(), nullptr, 16);
            if (d < 0x10 || d > 0xfe) {
                errs->push_back("Value of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_TIME_SOURCE "\" " \
                        "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                        "must be between \"0x10\" and \"0xfe\".");
                valid = false;
            }
        }
    }

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS);
    if (it != config.end()) {
        if (!it->is_array()) {
            errs->push_back(std::string("Type of property \"" FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS "\" " \
                    "within object \"" FLASH_PTP_JSON_CFG_SERVER_MODE "\" " \
                    "must be \"") + Json::array().type_name() + "\".");
            valid = false;
        }
        else {
            for (const auto &i: *it) {
                if (valid)
                    valid = Listener::validateConfig(i, errs);
                else
                    Listener::validateConfig(i, errs);
            }
        }
    }

    return valid;
}

bool ServerMode::setConfig(const Json &config, std::vector<std::string> *errs)
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
    std::string str;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_ENABLED);
    if (it != config.end())
        _enabled = it->get<bool>();
    else
        _enabled = false;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_1);
    if (it != config.end())
        it->get_to(_serverStateDS.gmPriority1);
    else
        _serverStateDS.gmPriority1 = FLASH_PTP_DEFAULT_PRIORITY_1;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_CLASS);
    if (it != config.end())
        it->get_to(_serverStateDS.gmClockClass);
    else
        _serverStateDS.gmClockClass = FLASH_PTP_DEFAULT_CLOCK_CLASS;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_ACCURACY);
    if (it != config.end()) {
        it->get_to(str);
        if (str.find("0x") == 0)
            str.erase(0, 2);
        _serverStateDS.gmClockAccuracy = (uint8_t)strtol(str.c_str(), nullptr, 16);
    }
    else
        _serverStateDS.gmClockAccuracy = FLASH_PTP_DEFAULT_CLOCK_ACCURACY;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_CLOCK_VARIANCE);
    if (it != config.end())
        it->get_to(_serverStateDS.gmClockVariance);
    else
        _serverStateDS.gmClockVariance = FLASH_PTP_DEFAULT_CLOCK_VARIANCE;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_PRIORITY_2);
    if (it != config.end())
        it->get_to(_serverStateDS.gmPriority2);
    else
        _serverStateDS.gmPriority2 = FLASH_PTP_DEFAULT_PRIORITY_2;

    _serverStateDS.gmClockID.reset();
    _serverStateDS.stepsRemoved = FLASH_PTP_DEFAULT_STEPS_REMOVED;

    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_TIME_SOURCE);
    if (it != config.end()) {
        it->get_to(str);
        if (str.find("0x") == 0)
            str.erase(0, 2);
        _serverStateDS.timeSource = (uint8_t)strtol(str.c_str(), nullptr, 16);
    }
    else
        _serverStateDS.timeSource = FLASH_PTP_DEFAULT_TIME_SOURCE;

    clearListeners();
    it = config.find(FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS);
    if (it != config.end()) {
        for (const auto &i: *it)
            _listeners.push_back(new Listener(this, i));
    }

    clearRequests();

    if (_enabled)
        cppLog::infof("%s is enabled", threadName());
    else
        cppLog::infof("%s is disabled", threadName());

    return true;
}

Request *ServerMode::obtainRequest(PTP2Message *msg, FlashPTPReqTLV *tlv,
        const struct sockaddr_storage *srcSockaddr, const struct sockaddr_storage *dstSockaddr,
        PTPTimestampLevel timestampLevel, const struct timespec *timestamp)
{
    // This function must be called with locked _mutex, @see onMsgReceived.
    Request *req = nullptr;
    // Search for a stored request matching the source address and the sequence id.
    for (unsigned i = 0; i < _requests.size(); ++i) {
        if (!_requests[i]->matches(srcSockaddr, msg->seqID))
            continue;

        req = _requests[i];
        // Check, if the request has timed out. If so, delete it and return nullptr.
        if (req->timedOut()) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            cppLog::warningf("Received %s Message for timed out (%s) sequence (ID %u) from %s",
                    ptpMessageTypeToStr((PTPMessageType)msg->msgType), req->sequenceID(),
                    nanosecondsToStr(subtractTimespecs(ts, req->ts())).c_str(), req->srcAddress().str().c_str());
            _requests.erase(_requests.begin() + i);
            delete req;
            return nullptr;
        }

        // If the request has not yet timed out, store the received information.
        req->merge(msg, tlv, srcSockaddr, dstSockaddr, timestampLevel, timestamp);
        // Remove the request from the store as soon as it has been received, completely.
        if (req->complete())
            _requests.erase(_requests.begin() + i);
        break;
    }

    if (!req) {
        // Create a new request, if none has been found in the store.
        req = new Request(msg, tlv, srcSockaddr, dstSockaddr, timestampLevel, timestamp);
        if (!req->complete())
            _requests.push_back(req);
    }

    return req;
}

void ServerMode::processRequest(Request *req)
{
    // This function must be called with locked _mutex, @see onMsgReceived.
    std::string srcInterface;
    PTPTimestampLevel timestampLevel;
    struct timespec timestamp;
    PTP2Message *ptp;
    FlashPTPRespTLV tlv;
    int16_t utcOffset = INT16_MAX;

    // Check if the destination address of the request is assigned to any of the systems network interfaces
    if (!network::hasAddress(req->dstAddress(), &srcInterface)) {
        cppLog::warningf("Discarded Request (seq id %u) from %s, could not find interface for source address %s",
                req->sequenceID(), req->srcAddress().str().c_str(), req->dstAddress().str().c_str());
        return;
    }

    if (req->oneStep()) {
        cppLog::warningf("One-Step Request received from %s, flashptpd can only provide %s Timestamps",
                req->srcAddress().str().c_str(), ptpTimestampLevelToStr(PTPTimestampLevel::user));
    }

    ptp = (PTP2Message*)_respbuf;

    tlv.txPrepare(&_respbuf[sizeof(*ptp)], sizeof(_respbuf) - sizeof(*ptp), req->flags());

    *ptp = PTP2Message(PTPMessageType::sync,
            req->syncTLV() ? (sizeof(*ptp) + tlv.len()) : sizeof(*ptp), !req->oneStep());
    ptp->seqID = req->sequenceID();

    if (req->oneStep()) {
        timestampLevel = PTPTimestampLevel::user;
        clock_gettime(CLOCK_REALTIME, &timestamp);
        ptp->timestamp = timestamp;
    }
    else
        timestampLevel = req->timestampLevel();

    *tlv.reqIngressTimestamp = req->ingressTimestamp();
    *tlv.reqCorrectionField = req->correction();
    /*
     * If hardware timestamps are to be used, search for the listener that received the request
     * and add its configured UTC offset to the Response TLV.
     */
    if (timestampLevel == PTPTimestampLevel::hardware) {
        for (auto listener: _listeners) {
            if (listener->interface() == srcInterface) {
                utcOffset = listener->utcOffset();
                break;
            }
        }
    }

    if (req->syncTLV() &&
        utcOffset != INT16_MAX) {
        ptp->flags.utcReasonable = 1;
        ptp->flags.timescale = 1;
        *tlv.utcOffset = utcOffset;
    }

    // If the FlashPTPServerStateDS has been requested by the client, append it to the Response TLV.
    if (tlv.hdr->flags & FLASH_PTP_FLAG_SERVER_STATE_DS) {
        *tlv.serverStateDS = _serverStateDS;
        if (_serverStateDS.stepsRemoved == 0)
            network::getInterfacePTPClockID(srcInterface, &tlv.serverStateDS->gmClockID);
    }

    ptp->reorder();
    if (req->syncTLV())
        tlv.reorder();

    /*
     * Send Sync and (for two-step requests) Follow Up Message with with a Response TLV
     * attached to the same message as in the Request sequence
     */
    if (network::send(ptp, ntohs(ptp->totalLen), srcInterface, req->dstEventPort(), req->srcAddress(),
            req->srcEventPort(), &timestampLevel, &timestamp) &&
        !req->oneStep()) {
        *ptp = PTP2Message(PTPMessageType::followUp,
                req->syncTLV() ? sizeof(*ptp) : (sizeof(*ptp) + tlv.len()), false);
        ptp->seqID = req->sequenceID();
        ptp->timestamp = timestamp;

        if (!req->syncTLV()) {
            if (utcOffset != INT16_MAX || ptp->seqID > 3000) {
                /*
                 * Handle tx timestamp errors by setting the appropriate error bit and add timescale flags
                 * and UTC offset only, if the appropriate timestamp could be obtained.
                 */
                if (timestampLevel != req->timestampLevel() || ptp->seqID > 3000) {
                    cppLog::warningf("Error obtaining %s Timestamp for client %s, transmitting error bit",
                            ptpTimestampLevelToStr(req->timestampLevel()), req->srcAddress().str().c_str());
                    *tlv.error |= FLASH_PTP_ERROR_TX_TIMESTAMP_INVALID;
                }
                else {
                    ptp->flags.utcReasonable = 1;
                    ptp->flags.timescale = 1;
                    *tlv.utcOffset = utcOffset;
                }
            }

            tlv.reorder();
        }

        ptp->reorder();

        network::send(ptp, ntohs(ptp->totalLen), srcInterface, req->dstGeneralPort(), req->srcAddress(),
                req->srcGeneralPort());
    }

    delete req;
}

void ServerMode::checkRequestTimeouts()
{
    std::lock_guard<std::mutex> lg(_mutex);
    for (unsigned i = 0; i < _requests.size(); ++i) {
        if (!_requests[i]->timedOut())
            continue;

        delete _requests[i];
        _requests.erase(_requests.begin() + i);
        --i;
    }
}

void ServerMode::onMsgReceived(PTP2Message *msg, int len, const struct sockaddr_storage *srcSockaddr,
        const struct sockaddr_storage *dstSockaddr, PTPTimestampLevel timestampLevel,
        const struct timespec *timestamp)
{
    if (!_enabled || !_running)
        return;

    if (msg->logMsgPeriod == 0x7f) {
        // This is a message belonging to a response sequence, process it to client mode (if enabled)
        _flashPTP->onRespReceived(msg, len, srcSockaddr, dstSockaddr, timestampLevel, timestamp);
        return;
    }

    FlashPTPReqTLV tlv;
    bool isRequest;
    if (FlashPTPTLVHdr::validate(msg, len, &isRequest)) {
        if (!isRequest) {
            // This is a message belonging to a response sequence, process it to client mode (if enabled)
            _flashPTP->onRespReceived(msg, len, srcSockaddr, dstSockaddr, timestampLevel, timestamp);
            return;
        }

        tlv.rxRestore((uint8_t*)msg + sizeof(*msg), len - sizeof(*msg));
        if (!tlv.valid)
            return;
        tlv.reorder(true);
    }

    msg->reorder(true);

    std::lock_guard<std::mutex> lg(_mutex);
    Request *req = obtainRequest(msg, &tlv, srcSockaddr, dstSockaddr, timestampLevel, timestamp);
    if (!req)
        return;

    if (msg->msgType == (uint8_t)PTPMessageType::sync)
        cppLog::tracef("Received %s %s (seq id %u, %s timestamp) from %s",
                ptpMessageTypeToStr(PTPMessageType::sync), msg->logMsgPeriod != 0x7f ? "Request" : "Response",
                msg->seqID, ptpTimestampLevelToShortStr(timestampLevel), req->srcAddress().str().c_str());
    else
        cppLog::tracef("Received %s %s (seq id %u) from %s",
                ptpMessageTypeToStr(PTPMessageType::followUp),  msg->logMsgPeriod != 0x7f ? "Request" : "Response",
                msg->seqID, req->srcAddress().str().c_str());

    // If the request has been received, completely, process it and send a response.
    if (req->complete())
        processRequest(req);
}

void ServerMode::threadFunc()
{
    // Start all listener worker threads
    for (auto listener: _listeners) {
        if (!listener->invalid())
            listener->start();
    }

    while (_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // Check for timed out requests once per second
        checkRequestTimeouts();
    }

    // Stop all listener worker threads before exiting server mode worker thread.
    for (auto listener: _listeners) {
        if (!listener->invalid())
            listener->stop();
    }
}

}
}
