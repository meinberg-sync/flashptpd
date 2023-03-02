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

void signalHandler(int signal)
{
    if (signal == SIGPIPE)
        return;

    __signalStatus = signal;
}

void printUsage()
{
    printf("%s v%s\n", FLASH_PTP_DAEMON, FLASH_PTP_VERSION);
    printf("Usage: [-c <str>] [-f] [-h]\n");
    printf("  %-6s%-16s%s\n", "-c", "--config", "flashPTP configuration file in JSON format");
    printf("  %-6s%-16s%s\n", "-s", "--servers", "name a file to periodically print a list of the servers and");
    printf("  %-6s%-16s%s\n", "", "", "a short summary of their current state to (client mode)");
    printf("  %-6s%-16s%s\n", "-f", "--fork", "fork service into background");
    printf("  %-6s%-16s%s\n", "-i", "--inventory", "show system inventory (interfaces, addresses, timestampers) and exit");
    printf("  %-6s%-16s%s\n", "-h", "--help", "print this usage information");
    printf("\n");
}

bool parseArgs(Json &config, std::string &configFile, std::string &serversFile, bool &inventory, bool &daemonize,
        int argc, char **argv)
{
    char *arg, *val, c;
    std::ifstream ifs;
    std::ofstream ofs;

    for (int i = 1; i < argc; ++i) {
        arg = argv[i];
        if (strlen(arg) < 2 || arg[0] != '-') {
            printf("Argument '%s' is invalid!\n", arg);
            return false;
        }

        if (arg[1] == '-') {
            if (strlen(arg) < 3) {
                printf("Argument '%s' is invalid!\n", arg);
                return false;
            }

            if (strcmp(&arg[2], "config") == 0)
                c = 'c';
            else if (strcmp(&arg[2], "servers") == 0)
                c = 's';
            else if (strcmp(&arg[2], "inventory") == 0)
                c = 'i';
            else if (strcmp(&arg[2], "fork") == 0)
                c = 'f';
            else if (strcmp(&arg[2], "help") == 0)
                c = 'h';
            else {
                printf("Argument '%s' is invalid!\n", arg);
                return false;
            }
        }
        else
            c = arg[1];

        switch (c) {
        case 'c':
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

            configFile = argv[i];
            break;

        case 's':
            ++i;
            if (i >= argc) {
                printf("No filename specified for argument '%s'!\n", arg);
                return false;
            }

            val = argv[i];
            ofs.open(val);
            if (!ofs.good()) {
                printf("Servers file '%s' could not be opened with write access!\n", val);
                return false;
            }

            ofs.close();
            serversFile = val;
            break;

        case 'f': daemonize = true; break;
        case 'i': inventory = true; break;
        case 'h': return false;
        default:
            printf("Argument '%s' is invalid!\n", arg);
            return false;
        }
    }

    return true;
}

int main(int argc, char **argv)
{
    std::vector<std::string> configErrs;
    std::string configFile, serversFile;
    Json config;
    bool inventory, daemonize;
    FlashPTP *f;

    __signalStatus = 0;

    inventory = false;
    daemonize = false;
    if (!parseArgs(config, configFile, serversFile, inventory, daemonize, argc, argv)) {
        printUsage();
        std::exit(EXIT_FAILURE);
    }

    if (inventory) {
        /*
         * initialize network, wait for inventory worker thread to complete (at least once),
         * print network interfaces, addresses and capabilities (PHCs), then exit.
         */
        int ms = 2000;
        network::init();
        while (ms > 0 && !network::initialized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ms -= 100;
        }
        network::print();
        network::exit();
        std::exit(EXIT_SUCCESS);
    }

    if (configFile.empty()) {
        printf("No config file specified!\n");
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
        printf("Config file '%s' is invalid:\n", configFile.c_str());
        for (const auto &str: configErrs)
            printf("%s\n", str.c_str());
        std::exit(EXIT_FAILURE);
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGPIPE, signalHandler);
    std::signal(SIGTERM, signalHandler);

    f->setConfig(config, configFile);
    if (!serversFile.empty())
        f->clientMode().setServersFile(serversFile);
    f->start();

    while (__signalStatus == 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    f->stop();
    delete f;

    std::exit(EXIT_SUCCESS);
}


