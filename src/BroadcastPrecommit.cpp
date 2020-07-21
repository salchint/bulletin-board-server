//Filename:  BroadcastPrecommit.cpp

#include "BroadcastPrecommit.h"
#include <cstring>
#include <cstdio>

void BroadcastPrecommit::execute()
{
    if (this->commandId != COMMAND_ID)
    {
        debug_print(this, "Command ", this->commandId, " is not for me");
        return;
    }

    debug_print(this, "Broadcasting ", COMMAND_ID, " command\n");
    
    //TODO Falscher stream >> peer!!!

    debug_print(this, "Broadcasting ", COMMAND_ID, " via socket ", fileno(this->stream));
    fprintf(this->stream, "%s\n", this->line + std::strlen("BROADCAST-"));
    fflush(this->stream);
}
