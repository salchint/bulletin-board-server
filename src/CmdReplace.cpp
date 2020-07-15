//Filename:  CmdReplace.cpp

#include "CmdReplace.h"
#include <cstring>
#include <string>
#include <string_view>
#include <fstream>
#include <climits>
#include <optional>
#include <sstream>
#include <array>
#include <cstdio>

bool replace_record(std::istream& input, std::ostream& output, size_t id, std::string_view substitute, std::string& user)
{
    auto replaced {false};
    std::stringstream sid;
    sid << id;
    input.seekg(0);
    output.seekp(0);

    for (std::string line; std::getline(input, line); )
    {
        if (0 == line.find(sid.str() + "/"))
        {
            output << id << "/" << user << "/" << substitute << std::endl;
            replaced = true;
            debug_print(&output, "!!!", substitute);
        }
        else
        {
            output << line << std::endl;
            debug_print(&output, line);
        }
    }

    return replaced;
}

void CmdReplace::execute()
{
    if (this->commandId != COMMAND_ID)
    {
        debug_print(this, "Command ", this->commandId, " is not for me");
        return;
    }

    debug_print(this, "Processing ", COMMAND_ID, " command\n");

    size_t id;
    char seperator;
    std::array<char, 1024> message;
    std::string dummy;
    std::stringstream sout(this->line);
    sout >> dummy;      // command REPLACE
    sout >> id;         // message ID
    sout >> seperator;  // seperator /
    sout.getline(message.data(), message.size());

    try
    {
        // Throw if the request does not contain a valid number
        if (sout.fail())
        {
            error_return(this, "Request malformed");
        }

        // Backup the original bbfile
        auto origName { Config::singleton().get_bbfile() };
        auto backupName { origName + "~"};
        std::rename(origName.data(), backupName.data());

        // Rewrite the bbfile
        std::ifstream input(backupName);
        std::ofstream output(origName, std::ios_base::out|std::ios_base::trunc);
        auto success { replace_record(input, output, id, message.data(), this->user) };

        // Throw if the file is absent or the copy operation failed
        if (output.fail())
        {
            error_return(this, "Failed to rewrite Bbfile: State = ",
                    name_statebits(output.rdstate()));
        }

        if (success)
        {
            fprintf(this->stream, "3.0 WROTE %ld\n", id);
        }
        else
        {
            fprintf(this->stream, "3.1 UNKNOWN %ld\n", id);
        }

    }
    catch (const BBServException& error)
    {
        fprintf(this->stream, "3.2 ERROR WRITE %s\n", error.what());
    }

    fflush(this->stream);
}