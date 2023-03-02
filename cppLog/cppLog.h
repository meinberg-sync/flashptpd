/*
 * @file cppLog.h
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
 *
 * cppLog is a single source/header file library that provides easy-to-use API
 * functions for stdout, file and syslog logging. It takes a Json-formatted
 * configuration (@see cppLog::init) and writes logs to the configured channels
 * with the configured severities.
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

#ifndef INCLUDE_CPP_LOG_H_
#define INCLUDE_CPP_LOG_H_

#include <mutex>
#include <shared_mutex>
#include <nlohmann/json.hpp>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#if defined (__GNUC__)
#include <syslog.h>
#endif

namespace cppLog {

#define CPP_LOG_CONFIG_INSTANCE_ENABLED     "enabled"
#define CPP_LOG_CONFIG_INSTANCE_SEVERITY    "severity"
#define CPP_LOG_CONFIG_INSTANCE_FILENAME    "filename"

using Json = nlohmann::json;

enum class LogType {
    invalid = 0,
    min,
    stdStreams = min,
    file,
    syslog,
    max = syslog
};

inline static const char *logTypeToStr(LogType type)
{
    switch (type) {
    case LogType::invalid: return "invalid";
    case LogType::stdStreams: return "standardStreams";
    case LogType::file: return "file";
    case LogType::syslog: return "syslog";
    default: return "unknown";
    }
}

inline static LogType logTypeFromStr(const char *str)
{
    for (int i = (int)LogType::min; i <= (int)LogType::max; ++i) {
        if (strcmp(str, logTypeToStr((LogType)i)) == 0)
            return (LogType)i;
    }

    return LogType::invalid;
}

enum class LogSeverity {
    invalid = 0,
    min,
    error = min,
    warning,
    info,
    debug,
    trace,
    eleven,
    max = eleven
};

inline static const char *logSeverityToStr(LogSeverity severity)
{
    switch (severity) {
    case LogSeverity::error: return "error";
    case LogSeverity::warning: return "warning";
    case LogSeverity::info: return "info";
    case LogSeverity::debug: return "debug";
    case LogSeverity::trace: return "trace";
    case LogSeverity::eleven: return "eleven";
    default: return "unknown";
    }
}

inline static LogSeverity logSeverityFromStr(const char *str)
{
    for (int i = (int)LogSeverity::min; i <= (int)LogSeverity::max; ++i) {
        if (strcmp(str, logSeverityToStr((LogSeverity)i)) == 0)
            return (LogSeverity)i;
    }

    return LogSeverity::invalid;
}

#if defined (__GNUC__)
inline static int logSeverityToSyslogFacility(LogSeverity severity)
{
    switch (severity) {
    case LogSeverity::error: return LOG_ERR;
    case LogSeverity::warning: return LOG_WARNING;
    case LogSeverity::info: return LOG_INFO;
    default: return LOG_DEBUG;
    }
};
#endif

using LogFuncExt = void(LogSeverity, const char *fmt, ...);
using LogFunc = void(const char *fmt, ...);

static const char *__cpp_log_default_config_str =
#include "default.json"
;

static const Json __cpp_log_default_config = Json::parse(__cpp_log_default_config_str);

class LogInstance
{
public:
    inline LogInstance(LogType type, LogSeverity severity, const std::string &filename = "") :
        _type{ type }, _severity{ severity }
    {
        _filename = filename;
        if (_filename.length())
            remove(_filename.c_str());
    }

    inline void write(LogSeverity severity, const char *fmt, va_list args)
    {
        if (_severity < severity)
            return;

        if (_type == LogType::stdStreams ||
            _type == LogType::file) {
            FILE *f;

            if (_type == LogType::stdStreams) {
                if (severity == LogSeverity::error)
                    f = stderr;
                else
                    f = stdout;
            }
            else
                f = fopen(_filename.c_str(), "a");

            if (f) {
                if (_type == LogType::stdStreams)
                    fprintf(f, "\r");

                time_t t = time(NULL);
                if (t >= 0) {
                    struct tm* tm = localtime(&t);
                    if (tm) {
                        strftime(_dbuf, sizeof(_dbuf), "%b %d %T ", tm);
                        fprintf(f, "%s", _dbuf);
                    }
                }

                fprintf(f, "%s: ", logSeverityToStr(severity));
                vfprintf(f, fmt, args);
                fprintf(f, "\n");

                if (_type == LogType::file)
                    fclose(f);
            }
        }
#if defined (__GNUC__)
        else if (_type == LogType::syslog)
            vsyslog(logSeverityToSyslogFacility(severity), fmt, args);
#endif
    }

    inline LogType type() const { return _type; }
    inline LogSeverity severity() const { return _severity; }
    inline const std::string &filename() const { return _filename; }

private:
    LogType _type{ LogType::invalid };
    LogSeverity _severity{ LogSeverity::debug };
    std::string _filename;

    char _dbuf[32];
};

class Log
{
public:
    inline Log(bool stdStreamsEnabled, LogSeverity stdStreamsSeverity,
               bool fileEnabled, LogSeverity fileSeverity, const char *filename,
               bool syslogEnabled, LogSeverity syslogSeverity,
               std::shared_mutex *mutex = nullptr)
    {
        _sm = mutex;
        if (!_sm) {
            _sm = new std::shared_mutex();
            _own = true;
        }

        if (stdStreamsEnabled)
            _instances.emplace_back(LogType::stdStreams, stdStreamsSeverity);

        if (fileEnabled)
            _instances.emplace_back(LogType::file, fileSeverity, filename);

        if (syslogEnabled)
            _instances.emplace_back(LogType::syslog, syslogSeverity);
    }

    inline Log(const Json &j = __cpp_log_default_config, std::shared_mutex *mutex = nullptr)
    {
        _sm = mutex;
        if (!_sm) {
            _sm = new std::shared_mutex();
            _own = true;
        }
        fromJson(j);
    }

    inline Log(const Json &j, std::recursive_mutex *mutex) : _rm{ mutex } { fromJson(j); }

    inline ~Log()
    {
        lock();
        _instances.clear();
        unlock();

        if (_sm && _own)
            delete _sm;
    }

    inline void lock(bool shared = false) const
    {
        if (_sm) {
            if (shared)
                _sm->lock_shared();
            else
                _sm->lock();
        }
        else if (_rm)
            _rm->lock();
    }

    inline void unlock(bool shared = false) const
    {
        if (_sm) {
            if (shared)
                _sm->unlock_shared();
            else
                _sm->unlock();
        }
        else if (_rm)
            _rm->unlock();
    }

    inline void fromJson(const Json &j)
    {
        Json::const_iterator it, iit;
        std::string str, filenameStr;
        LogSeverity severity;
        int t;

        lock();
        _instances.clear();

        for (t = (int)LogType::min; t <= (int)LogType::max; ++t) {
            it = j.find(logTypeToStr((LogType)t));
            if (it == j.end())
                continue;

            iit = it->find(CPP_LOG_CONFIG_INSTANCE_ENABLED);
            if (iit == it->end() || !iit->is_boolean() || !iit->get<bool>())
                continue;

            if ((LogType)t == LogType::file) {
                iit = it->find(CPP_LOG_CONFIG_INSTANCE_FILENAME);
                if (iit == it->end() || !iit->is_string())
                    continue;

                iit->get_to(filenameStr);
            }
            else
                filenameStr.clear();

            severity = LogSeverity::invalid;
            iit = it->find(CPP_LOG_CONFIG_INSTANCE_SEVERITY);
            if (iit != it->end() && iit->is_string()) {
                iit->get_to(str);
                severity = logSeverityFromStr(str.c_str());
            }

            if (severity == LogSeverity::invalid) {
#if defined (NDEBUG)
                if (t == (int)LogType::stdStreams ||
                    t == (int)LogType::file)
                    severity = LogSeverity::info;
                else
                    severity = LogSeverity::error;
#else
                severity = LogSeverity::debug;
#endif
            }

            _instances.emplace_back((LogType)t, severity, filenameStr.c_str());
        }

        unlock();
    }

    inline Json toJson() const
    {
        Json j = Json::object(), jj;
        lock();
        for (const auto &i: _instances) {
            jj = Json::object();
            jj[CPP_LOG_CONFIG_INSTANCE_ENABLED] = true;
            jj[CPP_LOG_CONFIG_INSTANCE_SEVERITY] = logSeverityToStr(i.severity());
            if (i.type() == LogType::file)
                jj[CPP_LOG_CONFIG_INSTANCE_FILENAME] = i.filename();
            j[logTypeToStr(i.type())] = jj;
        }
        unlock();
        return j;
    }

    inline bool hasSeverity(LogSeverity severity)
    {
        bool hasSeverity = false;
        lock(true);
        for (auto &i: _instances) {
            if (i.severity() >= severity) {
                hasSeverity = true;
                break;
            }
        }
        unlock(true);
        return hasSeverity;
    }

    inline void logf(LogSeverity severity, const char *fmt, ...)
    {
        if (!hasSeverity(severity))
            return;

        va_list args;
        lock();
        for (auto &i: _instances) {
            if (i.severity() < severity)
                continue;

            va_start(args, fmt);
            i.write(severity, fmt, args);
            va_end(args);
        }
        unlock();
    }

    inline void logf(LogSeverity severity, const char *fmt, va_list args)
    {
        va_list args_copy;
        lock();
        for (auto &i: _instances) {
            if (i.severity() < severity)
                continue;

            va_copy(args_copy, args);
            i.write(severity, fmt, args_copy);
            va_end(args_copy);
        }
        unlock();
    }

    inline void errorf(const char *fmt, ...)
    {
        if (!hasSeverity(LogSeverity::error))
            return;

        va_list args;
        lock();
        for (auto &i: _instances) {
            va_start(args, fmt);
            i.write(LogSeverity::error, fmt, args);
            va_end(args);
        }
        unlock();
    }

    inline void warningf(const char *fmt, ...)
    {
        if (!hasSeverity(LogSeverity::warning))
            return;

        va_list args;
        lock();
        for (auto &i: _instances) {
            if (i.severity() < LogSeverity::warning)
                continue;

            va_start(args, fmt);
            i.write(LogSeverity::warning, fmt, args);
            va_end(args);
        }
        unlock();
    }

    inline void infof(const char *fmt, ...)
    {
        if (!hasSeverity(LogSeverity::info))
            return;

        va_list args;
        lock();
        for (auto &i: _instances) {
            if (i.severity() < LogSeverity::info)
                continue;

            va_start(args, fmt);
            i.write(LogSeverity::info, fmt, args);
            va_end(args);
        }
        unlock();
    }

    inline void debugf(const char *fmt, ...)
    {
        if (!hasSeverity(LogSeverity::debug))
            return;

        va_list args;
        lock();
        for (auto &i: _instances) {
            if (i.severity() < LogSeverity::debug)
                continue;

            va_start(args, fmt);
            i.write(LogSeverity::debug, fmt, args);
            va_end(args);
        }
        unlock();
    }

    inline void tracef(const char *fmt, ...)
    {
        if (!hasSeverity(LogSeverity::trace))
            return;

        va_list args;
        lock();
        for (auto &i: _instances) {
            if (i.severity() < LogSeverity::trace)
                continue;

            va_start(args, fmt);
            i.write(LogSeverity::trace, fmt, args);
            va_end(args);
        }
        unlock();
    }

    inline void elevenf(const char *fmt, ...)
    {
        if (!hasSeverity(LogSeverity::eleven))
            return;

        va_list args;
        lock();
        for (auto &i: _instances) {
            if (i.severity() < LogSeverity::eleven)
                continue;

            va_start(args, fmt);
            i.write(LogSeverity::eleven, fmt, args);
            va_end(args);
        }
        unlock();
    }

private:
    std::vector<LogInstance> _instances;

    std::shared_mutex *_sm{ nullptr };
    std::recursive_mutex *_rm{ nullptr };
    bool _own{ false };
};

void init(const Json &config);
void errorf(const char *fmt, ...);
void warningf(const char *fmt, ...);
void infof(const char *fmt, ...);
void debugf(const char *fmt, ...);
void tracef(const char *fmt, ...);
void elevenf(const char *fmt, ...);
void exit();


}

#endif /* INCLUDE_CPP_LOG_H_ */
