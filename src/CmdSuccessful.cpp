//Filename:  CmdSuccessful.cpp

#include "CmdSuccessful.h"

void CmdSuccessful::execute()
{
    if (this->commandId != COMMAND_ID)
    {
        debug_print(this, "Command ", this->commandId, " is not for me");
        return;
    }

    debug_print(this, "Processing ", COMMAND_ID, " command\n");


    fprintf(this->stream, "ACK\n");
    fflush(this->stream);
}
