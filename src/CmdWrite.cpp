//Filename:  CmdWrite.cpp

#include "CmdWrite.h"
#include <cstring>
#include <string>
#include <string_view>
#include <fstream>
#include "ConnectionQueue.h"

size_t CmdWrite::update_message_number()
{
    auto &in =  std::ios_base::in;
    auto &out =  std::ios_base::out;
    auto &trunc =  std::ios_base::trunc;

    std::string noFile { Config::singleton().get_bbfile() + ".no" };
    std::fstream io(noFile.data());
    size_t number;

    if (io.fail())
    {
        io.open(noFile.data(), in|out|trunc);
        io << 0;
        io.flush();
        io.seekg(0);
    }

    io >> number;
    io.seekp(0);
    io.clear();
    io << number + 1;

    if (io.fail())
    {
        error_return(this, "Failed to read/write message number (", noFile, "), state=",
                name_statebits(io.rdstate()));
    }

    debug_print(this, "Current message number: ", number);
    return number;
}

bool CmdWrite::replicate_command(size_t messageId)
{
    BroadcastCommand broadcast;

    for (auto& peer : Config::singleton().get_peers())
    {
        debug_print(this, "Add PRECOMMIT/peer to connectionQueue: ", peer);
        broadcast.peer = peer;
        broadcast.command = "BROADCAST-PRECOMMIT ";
        broadcast.command += std::to_string(messageId);
        this->connectionQueue->add(broadcast);
    }

    // Wait for acknowledges from all peers
    if (!AcknowledgeQueue::TheOne(messageId)->check_success(Config::singleton().get_peers().size()))
    {
        debug_print(this, "Did not get positive acknowledge from all peers");
        return false;
    }
    debug_print(this, "All peers acknowledged");

    return true;
}

void CmdWrite::execute()
{
    auto &in =  std::ios_base::in;
    auto &out =  std::ios_base::out;
    auto &trunc =  std::ios_base::trunc;

    if (this->commandId != COMMAND_ID)
    {
        debug_print(this, "Command ", this->commandId, " is not for me");
        return;
    }

    debug_print(this, "Processing ", COMMAND_ID, " command\n");

    std::string message { this->line + std::strlen(COMMAND_ID) + 1 };

    try
    {
        // Get a new message ID and open the DB file
        auto id { update_message_number() };
        std::fstream fout (Config::singleton().get_bbfile());

        // Create the DB file if needed
        if (fout.fail())
        {
            debug_print(this, "Creating file: ",
                    Config::singleton().get_bbfile());
            fout.open(Config::singleton().get_bbfile(), in|out|trunc);
            fout.flush();
        }

        // Throw if this fails too
        if (fout.fail())
        {
            error_return(this, "Failed to open/create file ",
                    Config::singleton().get_bbfile()); }

        if (this->connectionQueue)
        {
            // Have all the peers process this command as well
            if (!replicate_command(id))
            {
                // TODO undo
            }
        }

        // Finally, the local write operation
        fout.seekp(0, std::ios_base::end);
        debug_print(this, "file pos ", fout.tellp());
        fout << id << "/" << this->user << "/" << message;

        fprintf(this->stream, "3.0 WROTE %lu\n", id);
        fflush(this->stream);
    }
    catch (const BBServException& error)
    {
        fprintf(this->stream, "3.2 ERROR WRITE %s\n", error.what());
        fflush(this->stream);
    }
}
