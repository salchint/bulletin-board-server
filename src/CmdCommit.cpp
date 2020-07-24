//Filename:  CmdCommit.cpp

#include "CmdCommit.h"
#include <iomanip>
#include <string>
#include <cstring>
#include <errno.h>
#include <sstream>
#include <cstdio>
#include "CmdBuilder.h"

void CmdCommit::execute()
{
    if (this->commandId != COMMAND_ID)
    {
        debug_print(this, "Command ", this->commandId, " is not for me");
        return;
    }

    debug_print(this, "Processing ", COMMAND_ID, " command");

    std::istringstream sin (this->line);
    std::string localCommandId;
    std::string localLine;
    std::string localUser;
    auto messageId {0};

    localCommandId.resize(100);
    localLine.resize(1024);
    localUser.resize(100);

    try
    {
        // Prepare a FILE stream for the locally invoked WRITE or REPLACE command
        auto pipeStream { StreamResource() };

        // Prepare the local WRITE or REPLACE command
        sin >> localCommandId >> localUser >> messageId;
        sin.ignore(1, ' ');
        sin.getline(localLine.data(), localLine.size(), '\n');
        localLine += "\n";

        if (sin.fail())
        {
            error_return(this, "Invalid COMMIT command");
        }

        std::istringstream sin2 (localLine);
        sin2 >> localCommandId;
        debug_print(this, "Local command: ", std::quoted(localCommandId), " ",
                localLine);

        SessionResources localResources;
        localResources.get_stream() = pipeStream.get_pipeStreams()[WRITE_END];
        localResources.get_user() = localUser;

        auto command { build_command(localCommandId, localLine.data(), localResources).value() };
        std::visit([](auto&& cmd) { cmd.execute(); }, command);

        fprintf(this->stream, "SUCCESSFUL\n");
        fflush(this->stream);
    }
    catch (const BBServException& error)
    {
        fprintf(this->stream, "UNSUCCESSFUL\n");
        fflush(this->stream);
    }

}
