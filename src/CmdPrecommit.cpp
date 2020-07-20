//Filename:  CmdPrecommit.cpp

#include "CmdPrecommit.h"
#include <sstream>

void CmdPrecommit::execute()
{
    if (this->commandId != COMMAND_ID)
    {
        debug_print(this, "Command ", this->commandId, " is not for me");
        return;
    }

    debug_print(this, "Processing ", COMMAND_ID, " command\n");

    auto messageId {0};
    std::istringstream sin(this->line);

    sin >> messageId;

    fprintf(this->stream, "ACK %d 1\n", messageId);
    fflush(this->stream);
}
