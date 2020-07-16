//Filename:  ThreadPool.cpp

#include "ThreadPool.h"
#include "Config.h"
#include <errno.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <array>
#include <optional>
#include <poll.h>
#include "SessionResources.h"

/**
 *Provide operator() for all availble commands.
 */
//template<class... Ts> struct Overload : Ts... { using Ts::operator()...; };
//template<class... Ts> Overload(Ts...) -> Overload<Ts...>;


/**
 *Print the supported commands as a wellcome message.
 */
static void print_commands(FILE* stream)
{
    fputs("0.0 bbserv supported commands: [USER <name>|READ <msg-number>|WRITE <msg>|REPLACE <msg-number>/<msg>|QUIT <text>]\n", stream);
    fflush(stream);
}

/**
 *Open a stream from the given socket.
 */
static void open_socket_stream(int socketNumber, FILE*& stream)
{
    stream = fdopen(socketNumber, "rb+");

    if (!stream) {
        error_return(stream, "Failed to open stream for socket ", socketNumber);
    }
}

/**
 * Build a command object for the given command ID.
 *
 * This function acts as a builder for all the supported command objects. It
 * returns a specific command object or std::nullopt if the requested command
 * is not supported.
 *
 * \param commandId Supported command identifiers include USER, WRITE, READ,
 *                  REPLACE and QUIT.
 * \param line  The received request message from the client. It is ought to
 *              start with the command identifier and may convey additional
 *              arguments.
 * \param resources Resources bound to this client connection including the
 *                  clients name as issued by the USER command.
 */
static std::optional<ThreadPool::Commands_t> build_command(const std::string& commandId, const char* line, SessionResources& resources)
{
    if (commandId == "USER")
    {
        return CmdUser(commandId, resources.get_stream(), line, resources.get_user());
    }
    else if (commandId == "WRITE")
    {
        return CmdWrite(commandId, resources.get_stream(), line, resources.get_user());
    }
    else if (commandId == "READ")
    {
        return CmdRead(commandId, resources.get_stream(), line, resources.get_user());
    }
    else if (commandId == "REPLACE")
    {
        return CmdReplace(commandId, resources.get_stream(), line, resources.get_user());
    }
    else if (commandId == "QUIT")
    {
        return CmdQuit(commandId, resources.get_stream(), line, resources.get_user());
    }
    return {};
}

/**
 *The thread's entry point.
 */
static void* thread_main(void* p)
{
    auto pool { reinterpret_cast<ThreadPool*>(p) };
    std::array<char, 16> commandId;
    std::array<char, 1024> line;

    for (;;)
    {
        SessionResources resources;
        commandId.fill('\0');
        line.fill('\0');
        resources.get_clientSocket() = pool->get_connection();

        try
        {
            open_socket_stream(resources.get_clientSocket(), resources.get_stream());
        }
        catch (const BBServException& error)
        {
            std::cout << "Stop processing client request on socket "
                << resources.get_clientSocket() << ": " << error.what()
                << std::endl;
            return nullptr;
        }

        print_commands(resources.get_stream());

        // In non-blocking mode, first check if there are data to be received.
        if (pool->get_timeout_ms())
        {
            pollfd descriptor;
            descriptor.fd = resources.get_clientSocket();
            descriptor.events = POLLIN;

            auto ready { poll(&descriptor, 1, pool->get_timeout_ms().value()) };

            if (0 == ready)
            {
                // timeout
                // TODO
            }
            else if (-1 == ready)
            {
                // error
                std::cout << "ERROR - Failed to poll for incoming data at socket "
                    << resources.get_clientSocket() << std::endl;
                return nullptr;
            }
        }

        while (fgets(line.data(), line.size(), resources.get_stream()))
        {
            debug_print(pool, "Received on ", resources.get_clientSocket(), ": ", line.data());

            sscanf(line.data(), "%s ", commandId.data());

            try
            {
                ThreadPool::Commands_t command { build_command(commandId.data(), line.data(), resources).value() };

                std::visit([](auto&& command) { command.execute(); }, command);

                if (0 == std::strncmp("QUIT", commandId.data(), 4))
                {
                    break;
                }

            }
            catch (const std::bad_optional_access&)
            {
                std::cout << "ERROR - Failed to build command object from unknown '" << line.data() <<"'" << std::endl;
                continue;
            }
            catch (const BBServException& error)
            {
                std::cout << error.what() << std::endl;

                // Shutdown in a civilized manner
                auto command { build_command("QUIT", "QUIT", resources).value() };
                std::visit([](auto&& command) { command.execute(); }, command);
                break;
            }

        }

        debug_print(pool, "Client connection closed on ", resources.get_clientSocket());
    }

    return nullptr;
}

static void create_pool(size_t size, std::vector<pthread_t>& container, ThreadPool* pool)
{
    pthread_attr_t clientThreadOptions;
    pthread_attr_init(&clientThreadOptions);
    pthread_attr_setdetachstate(&clientThreadOptions, PTHREAD_CREATE_DETACHED);

    for (auto i {0u}; i < size; ++i)
    {
        bool success { 0 == pthread_create(&container[i], &clientThreadOptions,
                thread_main, static_cast<void*>(pool))};

        if (!success)
        {
            error_return(pool, "Failed to create thread pool entry: ", strerror(errno));
        }
    }

    pthread_attr_destroy(&clientThreadOptions);
    debug_print(pool, size, " agents created and ready");
}

void ThreadPool::operate(std::shared_ptr<ConnectionQueue>& qu)
{
    this->connectionQueue = qu;
    create_pool(this->size, this->pool, this);

}

int ThreadPool::get_connection() noexcept
{
    return this->connectionQueue->get();
}

std::optional<int> ThreadPool::get_timeout_ms() noexcept
{
    return this->connectionQueue->get_timeout_ms();
}
