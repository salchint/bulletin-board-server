// config.h

#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <sstream>
#include "BBServException.h"

class Config {
    protected:
        std::string bbfile;
        size_t Tmax = 20;
        int bport = 9000;
        int sport = 10000;
        bool isDaemon = true;
        bool isDebug = false;

    public:
        static Config& singleton() {
            static Config object;
            return object;
        }

    public:
        /**
         *Returns the path to the bulletin board file.
         */
        const std::string& get_bbfile()   { return bbfile; }
        /**
         *Returns the number of used threads.
         */
        size_t get_Tmax()                 { return Tmax; }
        /**
         *Returns the port number of client-server communication.
         */
        int get_bport()                   { return bport;}
        /**
         *Returns the port number used for inter-server communication.
         */
        int get_sport()                   { return sport;}
        /**
         *Returns if the bbserv shall act as a daemon.
         */
        bool is_daemon()                  { return isDaemon; }
        /**
         *Returns if the debug messages shall be displayed.
         */
        bool is_debug()                   { return isDebug; }

        /**
         *Set the path to the bulletin board file.
         */
        void set_bbfile(const std::string& bbfile)  { this->bbfile = bbfile; }
        /**
         *Set the number of used threads.
         */
        void set_Tmax(size_t Tmax)                  { this->Tmax = Tmax; }
        /**
         *Set the port number of client-server communication.
         */
        void set_bport(int bport)                   { this->bport = bport; }
        /**
         *Set the port number used for inter-server communication.
         */
        void set_sport(int sport)                   { this->sport = sport;}
        /**
         *Set if bbserv shall act as a daemon.
         */
        void set_daemon(bool on)                    { this->isDaemon = on; }
        /**
         *Set of debug messages shall be displayed.
         */
        void set_debug(bool on)                     { this->isDebug = on; }
};

/**
 *Print error messages.
 */
template<typename OriginT, typename... ArgsT>
void error_return(OriginT origin, ArgsT... args)
{
    if (Config::singleton().is_debug())
    {
        std::stringstream sout;
        sout << "ERROR - " << typeid(origin).name() << ": ";
        (sout << ... << args) << ' ' << std::endl;
        throw BBServException(sout.str());
    }
}

/**
 *Print debug messages.
 */
template<typename OriginT, typename... ArgsT>
void debug_print(OriginT origin, ArgsT... args)
{
    if (Config::singleton().is_debug())
    {
        std::stringstream sout;
        std::cout << "DBG   - " << typeid(origin).name() << ": ";
        (std::cout << ... << args) << ' ' << std::endl;
    }
}

