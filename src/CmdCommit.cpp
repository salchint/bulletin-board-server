//Filename:  CmdCommit.cpp

#include "CmdCommit.h"
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

    debug_print(this, "Processing ", COMMAND_ID, " command\n");

    constexpr int READ_END {0};
    constexpr int WRITE_END {1};
    int pipeNo[2];
    FILE* pipeStream[2];
    std::istringstream sin (this->line);
    std::string localCommandId;
    std::string localLine;
    std::string localUser;
    SessionResources resources;

    localCommandId.resize(100);
    localLine.resize(1024);
    localUser.resize(100);
    pipeStream[READ_END] = nullptr;
    pipeStream[WRITE_END] = nullptr;

    try
    {
        // Prepare a FILE stream for the locally invoked WRITE or REPLACE command
        if (0 != pipe(pipeNo))
        {
            error_return(this, "Internal error on creating pipe: ", strerror(errno));
        }

        pipeStream[READ_END] = fdopen(pipeNo[READ_END], "r");
        pipeStream[WRITE_END] = fdopen(pipeNo[WRITE_END], "w");

        // Prepare the local WRITE or REPLACE command
        sin >> localCommandId.data() >> localUser.data() >> localCommandId.data();
        sin.getline(localLine.data(), localLine.size(), '\n');

        if (sin.fail())
        {
            error_return(this, "Invalid COMMIT command");
        }

        SessionResources localResources;
        localResources.get_stream() = pipeStream[WRITE_END];
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

    if (pipeStream[READ_END])
    {
        fclose(pipeStream[READ_END]);
    }
}
