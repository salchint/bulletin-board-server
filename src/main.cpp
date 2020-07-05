#include <iostream>
#include <unistd.h>
#include "config.h"

void print_usage() {
    std::cout << "bbserv - Bulletin Board Server" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: bbserv [arguments] [peer..]" << std::endl;
    std::cout << "  Peers must have the format 'host:port'. 0 or more are" << std::endl;
    std::cout << "  allowed after the arguments." << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  -h Print this usage information." << std::endl;
    std::cout << "  -b overrides (or sets) the file name bbfile according to its argument." << std::endl;
    std::cout << "  -T overrides THMAX according to its argument." << std::endl;
    std::cout << "  -p overrides the port number bp according to its argument." << std::endl;
    std::cout << "  -s overrides the port number sp according to its argument." << std::endl;
    std::cout << "  -f (with no argument) forces d to false." << std::endl;
    std::cout << "  -d (with no argument) forces D to true." << std::endl;
}

int main(int argc, char *argv[]) {
    const char* optionString { "hb:T:p:s:fd" };
    char option { '\0' };

    while (-1 != (option = getopt(argc, argv, optionString))) {
        switch (option) {
            case 'h':
            case '?':
                print_usage();
                return 0;
            case 'b':
                Config::singleton().set_bbfile(optarg);
                break;
            case 'T':
                Config::singleton().set_Tmax(atoi(optarg));
                break;
            case 'p':
                Config::singleton().set_bport(atoi(optarg));
                break;
            case 's':
                Config::singleton().set_sport(atoi(optarg));
                break;
            case 'f':
                Config::singleton().set_daemon(false);
                break;
            case 'd':
                Config::singleton().set_debug(true);
                break;
            default:
                break;
        }
    }

    return 0;
}
