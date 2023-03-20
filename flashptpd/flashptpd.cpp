/*
 * @file flashptpd.cpp
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * flashptpd is a command line utility which uses the flashPTP library to run
 * client and/or server mode of flashPTP on Linux computers and (if running in
 * client mode) to synchronize PTP hardware clocks (PHCs) and/or the system
 * clock (CLOCK_REALTIME) of the system it runs on.
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

using namespace flashptp;
volatile std::sig_atomic_t __signalStatus;

enum class CmdLineArg {
    invalid = -1,
    min,
    configFile = min,
    interface,
    destAddress,
    reqInterval,
    luckyPacket,
    stateInterval,
    ptpVersion,
    serverMode,
    utcOffset,
    networkProtocol,
    timestampLevel,
    logLevel,
    standardOut,
    noSyslog,
    logFile,
    stateFile,
    stateTable,
    printInventory,
    fork,
    help,
    max = help
};

static const char __arg_chars[] = {
    'c',
    'i',
    'd',
    'r',
    'z',
    'g',
    'v',
    'e',
    'u',
    'n',
    't',
    'l',
    'm',
    'q',
    'f',
    's',
    'x',
    'p',
    'f',
    'h'
};

static const char *__arg_strs[] = {
    "configFile",
    "interface",
    "destAddress",
    "reqInterval",
    "luckyPacket",
    "stateInterval",
    "ptpVersion",
    "serverMode",
    "utcOffset",
    "networkProtocol",
    "timestampLevel",
    "logLevel",
    "standardOut",
    "noSyslog",
    "logFile",
    "stateFile",
    "stateTable",
    "printInventory",
    "fork",
    "help"
};

static const char *__arg_descs[] = {
    "read configuration from file (JSON)",
    "network interface to be used (e.g., \"enp1s0\")",
    "server destination address in client mode (MAC, IPv4 or IPv6)",
    "interval to be used for external server requests (2^n)",
    "enable and set size of lucky packet filter",
    "interval to be used for external server state queries (2^n)",
    "PTP version to be used for server requests (v2/v2.1)",
    "enable server mode on the specified network interface",
    "offset to UTC in seconds (to be announced in server mode)",
    "network protocol to be used in server mode (if not any)",
    "fixed timestamp level to be used (hw/so/usr)",
    "set the log level for all enabled channels",
    "print logs to stdout",
    "do not print logs to syslog",
    "print logs to specified file",
    "periodically print the server state table to file (client mode)",
    "print the server state table to stdout (and disable stdout logs)",
    "print system inventory (interfaces, addresses, timestampers) and exit",
    "fork service into background",
    "print this usage information"
};

static CmdLineArg cmdLineArgFromChar(char c)
{
    for (int i = (int)CmdLineArg::min; i <= (int)CmdLineArg::max; ++i) {
        if (__arg_chars[i] == c)
            return (CmdLineArg)i;
    }
    return CmdLineArg::invalid;
}

static CmdLineArg cmdLineArgFromStr(const char *str)
{
    for (int i = (int)CmdLineArg::min; i <= (int)CmdLineArg::max; ++i) {
        if (strcasecmp(__arg_strs[i], str) == 0)
            return (CmdLineArg)i;
    }
    return CmdLineArg::invalid;
}

void printUsage()
{
    printf("%s v%s\n", FLASH_PTP_DAEMON, FLASH_PTP_VERSION);
    printf("Usage:\n");
    for (int i = (int)CmdLineArg::min; i <= (int)CmdLineArg::max; ++i)
        printf("  -%c    --%-18s%s\n", __arg_chars[i], __arg_strs[i], __arg_descs[i]);
    printf("\n");
}

bool parseArgs(Json &config, bool &inventory, bool &daemonize, int argc, char **argv)
{
    char *arg, *val, c;
    std::ifstream ifs;
    std::ofstream ofs;
    Json log;
    cppLog::LogSeverity sev;
    std::string intf;
    int8_t ri, si;
    network::Address destAddr;
    PTPVersion vers;
    PTPTimestampLevel tslvl;
    PTPProtocol prot;
    bool srv, sttbl;
    int utc;
    CmdLineArg a;
    unsigned lpf;

    for (int i = 1; i < argc; ++i) {
        arg = argv[i];
        if (strlen(arg) < 2 || arg[0] != '-') {
            printf("Argument '%s' is invalid!\n", arg);
            return false;
        }

        if (arg[1] == '-')
            a = cmdLineArgFromStr(&arg[2]);
        else
            a = cmdLineArgFromChar(arg[1]);

        if (a == CmdLineArg::invalid) {
            printf("Argument '%s' is invalid!\n", arg);
            return false;
        }
        else if (a != CmdLineArg::configFile) {
            if (a != CmdLineArg::serverMode &&
                a != CmdLineArg::standardOut &&
                a != CmdLineArg::noSyslog &&
                a != CmdLineArg::stateTable &&
                a != CmdLineArg::printInventory &&
                a != CmdLineArg::fork &&
                a != CmdLineArg::help)
                ++i;
            continue;
        }

        ++i;
        if (i >= argc) {
            printf("No filename specified for argument '%s'!\n", arg);
            return false;
        }

        val = argv[i];
        ifs.open(val);
        if (!ifs.good()) {
            printf("Config file '%s' could not be opened!\n", val);
            return false;
        }

        config = Json::parse(ifs, nullptr, false);
        ifs.close();

        if (config.is_discarded()) {
            printf("Config file '%s' is of invalid format!\n", val);
            return false;
        }

        break;
    }

    if (config.is_null())
        config = Json::object();

    log = Json::object();
    log[cppLog::logTypeToStr(cppLog::LogType::stdStreams)][CPP_LOG_CONFIG_INSTANCE_ENABLED] = false;
    log[cppLog::logTypeToStr(cppLog::LogType::file)][CPP_LOG_CONFIG_INSTANCE_ENABLED] = false;
    log[cppLog::logTypeToStr(cppLog::LogType::syslog)][CPP_LOG_CONFIG_INSTANCE_ENABLED] = true;

    ri = 0;
    lpf = 0;
    si = INT8_MAX;
    vers = PTPVersion::invalid;
    tslvl = PTPTimestampLevel::invalid;
    prot = PTPProtocol::invalid;
    srv = false;
    sttbl = false;
    utc = 0;

    for (int i = 1; i < argc; ++i) {
        arg = argv[i];
        if (strlen(arg) < 2 || arg[0] != '-') {
            printf("Argument '%s' is invalid!\n", arg);
            return false;
        }

        if (arg[1] == '-')
            a = cmdLineArgFromStr(&arg[2]);
        else
            a = cmdLineArgFromChar(arg[1]);

        switch (a) {
        case CmdLineArg::configFile: ++i; continue;

        case CmdLineArg::interface:
            ++i;
            if (i >= argc) {
                printf("No interface name specified for argument '%s'!\n", arg);
                return false;
            }

            intf = argv[i];
            if (!network::hasInterface(intf)) {
                printf("Interface '%s' could not be found!\n", intf.c_str());
                return false;
            }
            break;

        case CmdLineArg::destAddress:
            ++i;
            if (i >= argc) {
                printf("No destination address specified for argument '%s'!\n", arg);
                return false;
            }

            destAddr = network::Address(argv[i]);
            if (!destAddr.valid()) {
                printf("'%s' is not a valid destination (MAC, IPv4 or IPv6) address!\n", argv[i]);
                return false;
            }
            break;

        case CmdLineArg::reqInterval:
            ++i;
            if (i >= argc) {
                printf("No request interval specified for argument '%s'!\n", arg);
                return false;
            }

            ri = atoi(argv[i]);
            if (ri < -7 || ri > 7) {
                printf("'%s' is not a valid request interval (-7 <= n <= +7)\n", argv[i]);
                return false;
            }
            break;

        case CmdLineArg::luckyPacket:
            ++i;
            if (i >= argc) {
                printf("No filter size specified for argument '%s'!\n", arg);
                return false;
            }

            lpf = atoi(argv[i]);
            if (lpf <= 1) {
                printf("'%s' is not a valid filter size (1 < n)\n", argv[i]);
                return false;
            }
            break;

        case CmdLineArg::stateInterval:
            ++i;
            if (i >= argc) {
                printf("No state interval specified for argument '%s'!\n", arg);
                return false;
            }

            si = atoi(argv[i]);
            if (si < -7 || si > 7) {
                printf("'%s' is not a valid state interval (-7 <= n <= +7)\n", argv[i]);
                return false;
            }
            break;

        case CmdLineArg::ptpVersion:
            ++i;
            if (i >= argc) {
                printf("No PTP version specified for argument '%s'!\n", arg);
                return false;
            }

            vers = ptpVersionFromStr(argv[i]);
            if (vers != PTPVersion::v2_0 && vers != PTPVersion::v2_1) {
                printf("'%s' is not a valid PTP version (v2/v2.1)\n", argv[i]);
                return false;
            }
            break;

        case CmdLineArg::serverMode: srv = true; break;

        case CmdLineArg::utcOffset:
            ++i;
            if (i >= argc) {
                printf("No UTC offset specified for argument '%s'!\n", arg);
                return false;
            }

            utc = atoi(argv[i]);
            if (utc < INT16_MIN || utc > INT16_MAX) {
                printf("'%s' is not a valid UTC offset (%d <= n <= %d)\n", argv[i], INT16_MIN, INT16_MAX);
                return false;
            }
            break;

        case CmdLineArg::networkProtocol:
            ++i;
            if (i >= argc) {
                printf("No network protocol specified for argument '%s'!\n", arg);
                return false;
            }

            prot = ptpProtocolFromStr(argv[i]);
            if (prot == PTPProtocol::invalid) {
                printf("'%s' is not a valid network protocol (%s)\n", argv[i],
                        enumClassToStr<PTPProtocol>(ptpProtocolToShortStr).c_str());
                return false;
            }
            break;

        case CmdLineArg::timestampLevel:
            ++i;
            if (i >= argc) {
                printf("No timestamp level specified for argument '%s'!\n", arg);
                return false;
            }

            tslvl = ptpTimestampLevelFromShortStr(argv[i]);
            if (tslvl == PTPTimestampLevel::invalid) {
                printf("'%s' is not a valid timestamp level (%s)\n", argv[i],
                        enumClassToStr<PTPTimestampLevel>(ptpTimestampLevelToShortStr).c_str());
                return false;
            }
            break;

        case CmdLineArg::logLevel:
            ++i;
            if (i >= argc) {
                printf("No log level specified for argument '%s'!\n", arg);
                return false;
            }

            val = argv[i];
            sev = cppLog::logSeverityFromStr(val);
            if (sev == cppLog::LogSeverity::invalid) {
                printf("'%s' is not a valid log level: %s\n", val,
                        enumClassToStr<cppLog::LogSeverity>(cppLog::logSeverityToStr).c_str());
                return false;
            }

            log[cppLog::logTypeToStr(cppLog::LogType::stdStreams)][CPP_LOG_CONFIG_INSTANCE_SEVERITY] = val;
            log[cppLog::logTypeToStr(cppLog::LogType::file)][CPP_LOG_CONFIG_INSTANCE_SEVERITY] = val;
            log[cppLog::logTypeToStr(cppLog::LogType::syslog)][CPP_LOG_CONFIG_INSTANCE_SEVERITY] = val;
            break;

        case CmdLineArg::standardOut:
            log[cppLog::logTypeToStr(cppLog::LogType::stdStreams)][CPP_LOG_CONFIG_INSTANCE_ENABLED] = true;
            break;

        case CmdLineArg::noSyslog:
            log[cppLog::logTypeToStr(cppLog::LogType::syslog)][CPP_LOG_CONFIG_INSTANCE_ENABLED] = false;
            break;

        case CmdLineArg::logFile:
            ++i;
            if (i >= argc) {
                printf("No filename specified for argument '%s'!\n", arg);
                return false;
            }

            val = argv[i];
            ofs.open(val);
            if (!ofs.good()) {
                printf("Log file '%s' could not be opened with write access!\n", val);
                return false;
            }

            ofs.close();
            log[cppLog::logTypeToStr(cppLog::LogType::file)][CPP_LOG_CONFIG_INSTANCE_ENABLED] = true;
            log[cppLog::logTypeToStr(cppLog::LogType::file)][CPP_LOG_CONFIG_INSTANCE_FILENAME] = val;
            break;

        case CmdLineArg::stateFile:
            ++i;
            if (i >= argc) {
                printf("No filename specified for argument '%s'!\n", arg);
                return false;
            }

            val = argv[i];
            ofs.open(val);
            if (!ofs.good()) {
                printf("State file '%s' could not be opened with write access!\n", val);
                return false;
            }

            ofs.close();
            config[FLASH_PTP_JSON_CFG_CLIENT_MODE][FLASH_PTP_JSON_CFG_CLIENT_MODE_STATE_FILE] = val;
            break;

        case CmdLineArg::stateTable: sttbl = true; break;
        case CmdLineArg::printInventory: inventory = true; break;
        case CmdLineArg::fork: daemonize = true; break;
        case CmdLineArg::help: return false;
        default:
            printf("Argument '%s' is invalid!\n", arg);
            return false;
        }
    }

    if (destAddr.valid()) {
        Json clientMode = Json::object();
        Json server = Json::object();
        std::string phc;

        if (intf.empty()) {
            printf("Network interface must be specified ('-%c')!\n", __arg_chars[(int)CmdLineArg::interface]);
            return false;
        }

        server[FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_DST_ADDRESS] = destAddr.str();
        server[FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_SRC_INTERFACE] = intf;
        server[FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_REQUEST_INTERVAL] = ri;
        if (si != INT8_MAX)
            server[FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_STATE_INTERVAL] = si;
        if (vers != PTPVersion::invalid)
            server[FLASH_PTP_JSON_CFG_SERVER_MODE_SERVER_PTP_VERSION] = ptpVersionToShortStr(vers);
        if (tslvl != PTPTimestampLevel::invalid)
            server[FLASH_PTP_JSON_CFG_SERVER_MODE_SERVER_TIMESTAMP_LEVEL] = ptpTimestampLevelToShortStr(tslvl);

        if (lpf > 1) {
            Json filter = Json::object();
            filter[FLASH_PTP_JSON_CFG_FILTER_TYPE] = filter::Filter::typeToStr(filter::FilterType::luckyPacket);
            filter[FLASH_PTP_JSON_CFG_FILTER_SIZE] = lpf;

            server[FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS] = Json::array();
            server[FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVER_FILTERS].push_back(std::move(filter));
        }

        clientMode[FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS] = Json::array();
        clientMode[FLASH_PTP_JSON_CFG_CLIENT_MODE_SERVERS].push_back(std::move(server));

        network::getInterfacePHCInfo(intf, &phc, nullptr);
        if (!phc.empty()) {
            Json adjustment = Json::object();
            adjustment[FLASH_PTP_JSON_CFG_ADJUSTMENT_TYPE] =
                    adjustment::Adjustment::typeToStr(adjustment::AdjustmentType::pidController);
            adjustment[FLASH_PTP_JSON_CFG_ADJUSTMENT_CLOCK] = phc;

            clientMode[FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS] = Json::array();
            clientMode[FLASH_PTP_JSON_CFG_CLIENT_MODE_ADJUSTMENTS].push_back(std::move(adjustment));
        }

        clientMode[FLASH_PTP_JSON_CFG_CLIENT_MODE_STATE_TABLE] = sttbl;
        clientMode[FLASH_PTP_JSON_CFG_CLIENT_MODE_ENABLED] = true;

        config[FLASH_PTP_JSON_CFG_CLIENT_MODE] = std::move(clientMode);
    }

    if (srv) {
        Json serverMode = Json::object();
        Json listener = Json::object();

        if (intf.empty()) {
            printf("Network interface must be specified ('-%c')!\n", __arg_chars[(int)CmdLineArg::interface]);
            return false;
        }

        listener[FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_INTERFACE] = intf;
        if (prot != PTPProtocol::invalid)
            listener[FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_PROTOCOL] = ptpProtocolToShortStr(prot);
        if (tslvl != PTPTimestampLevel::invalid)
            listener[FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_TIMESTAMP_LEVEL] = ptpTimestampLevelToShortStr(tslvl);
        listener[FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENER_UTC_OFFSET] = utc;

        serverMode[FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS] = Json::array();
        serverMode[FLASH_PTP_JSON_CFG_SERVER_MODE_LISTENERS].push_back(std::move(listener));

        serverMode[FLASH_PTP_JSON_CFG_SERVER_MODE_ENABLED] = true;

        config[FLASH_PTP_JSON_CFG_SERVER_MODE] = std::move(serverMode);
    }

    if (sttbl)
        log[cppLog::logTypeToStr(cppLog::LogType::stdStreams)][CPP_LOG_CONFIG_INSTANCE_ENABLED] = false;
    config[FLASH_PTP_JSON_CFG_LOGGING] = std::move(log);

    return true;
}

void signalHandler(int signal)
{
    if (signal == SIGPIPE)
        return;

    __signalStatus = signal;
}

int main(int argc, char **argv)
{
    std::vector<std::string> configErrs;
    bool inventory, daemonize;
    Json config;
    FlashPTP *f;

    __signalStatus = 0;

    // initialize network and wait for inventory worker thread to complete (at least once)
    int ms = 2000;
    network::init();
    while (ms > 0 && !network::initialized()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ms -= 100;
    }

    inventory = false;
    daemonize = false;
    if (!parseArgs(config, inventory, daemonize, argc, argv)) {
        printUsage();
        std::exit(EXIT_FAILURE);
    }

    if (inventory) {
        // print network interfaces, addresses and capabilities (PHCs) and exit
        network::print();
        network::exit();
        std::exit(EXIT_SUCCESS);
    }

    if (config.empty()) {
        printf("No config (file or command line arguments) specified!\n");
        printUsage();
        std::exit(EXIT_FAILURE);
    }

    if (daemonize) {
        pid_t pid = fork();
        switch (pid) {
        case -1: {
            printf("Forking failed\n");
            std::exit(EXIT_FAILURE);
        }
        case 0: {
            if(setsid() == (pid_t)-1) {
                printf("setsid() failed\n");
                std::exit(EXIT_FAILURE);
            }
            break;
        }
        default: {
            printf("flashptpd forked to child process %d\n", pid);
            std::exit(EXIT_SUCCESS);
        }
        }
    }

    f = new FlashPTP();

    if (!f->validateConfig(config, &configErrs)) {
        printf("Configuration is invalid:\n");
        for (const auto &str: configErrs)
            printf("%s\n", str.c_str());
        std::exit(EXIT_FAILURE);
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGPIPE, signalHandler);
    std::signal(SIGTERM, signalHandler);

    f->setConfig(config);
    f->start();

    while (__signalStatus == 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    f->stop();
    delete f;

    network::exit();

    std::exit(EXIT_SUCCESS);
}


