//Filename:  BroadcastCommit.cpp

#include "BroadcastCommit.h"
#include <cstring>
#include <cstdio>
#include "CmdBuilder.h"

void BroadcastCommit::execute()
{
    if (this->commandId != COMMAND_ID)
    {
        debug_print(this, "Command ", this->commandId, " is not for me");
        return;
    }

    debug_print(this, "Broadcasting ", COMMAND_ID, " command");

    std::array<char, 1024> localLine;
    std::array<char, 100> localCommandId;

    //debug_print(this, "Broadcasting ", COMMAND_ID, " via socket ", fileno(this->stream));
    fprintf(this->stream, "%s\n", this->line + std::strlen("BROADCAST-"));

    std::fgets(localLine.data(), localLine.size(), this->stream);
    debug_print(this, "Processing reply: ", localLine.data(), " received via ", fileno(this->stream));

    // SUCCESSFUL or UNSUCCESSFUL
    std::istringstream sin(localLine.data());
    sin >> localCommandId.data();

    if (sin.fail())
    {
        error_return(this, "Invalid command reply; state=",
                name_statebits(sin.rdstate()));
    }

    SessionResources localResources;
    localResources.get_stream() = this->stream;

    auto command { build_command(commandId.data(), localLine.data(), localResources).value() };
    std::visit([](auto&& command) { command.execute(); }, command);

    localResources.detach_stream();
}
