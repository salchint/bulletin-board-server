//Filename:  CmdUser.h

#pragma once

#include <string>
#include "SessionResources.h"

/**
 *Command supporting the USER message.
 */
class CmdUser
{
    protected:
        constexpr static const char* const  COMMAND_ID { "USER" };
        std::string commandId;
        std::iostream& io;
        std::string& user;

    public:
        /**
         *Custom constructor accepting the command line received on the socket.
         */
        CmdUser(const std::string& commandId, std::iostream& io, std::string& user)
            : commandId(commandId)
            , io(io)
            , user(user)
        { }

    public:
        /**
         *Process the given command.
         */
        void execute();

};

