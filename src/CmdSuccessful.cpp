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

    auto messageId {0};
    std::array<char, 1024> localLine;
    std::array<char, 100> localCommandId;

    try
    {
        std::fgets(localLine.data(), localLine.size(), this->stream);
        std::istringstream sin(localLine.data());

        sin >> localCommandId.data() >> messageId;

        if (sin.fail())
        {
            error_return(this, "Invalid command reply; state=",
                    name_statebits(sin.rdstate()));
        }
    }
    catch (const BBServException& error)
    {
        AcknowledgeQueue::TheOne(messageId)->add(false);
    }

    AcknowledgeQueue::TheOne(messageId)->add(true);
}
