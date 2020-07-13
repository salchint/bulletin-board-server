//Filename:  CmdUser.cpp

#include "CmdUser.h"

void CmdUser::execute()
{
    if (this->commandId != COMMAND_ID)
    {
        debug_print(this, "Command ", this->commandId, " is not for me");
        return;
    }

    this->io >> this->user;
    debug_print(this, "Say HELLO to ", this->user);
    this->io.seekp(0);
    this->io << "1.0 HELLO " << this->user << " I'm ready" << std::endl;
    this->io.flush();
}
