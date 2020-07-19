//Filename:  CmdPrecommit.cpp

#include "CmdPrecommit.h"

void CmdPrecommit::execute()
{
    if (this->commandId != COMMAND_ID)
    {
        debug_print(this, "Command ", this->commandId, " is not for me");
        return;
    }

    debug_print(this, "Processing ", COMMAND_ID, " command\n");


    fprintf(this->stream, "ACK 1\n");
    fflush(this->stream);
}
