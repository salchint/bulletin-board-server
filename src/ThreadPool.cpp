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
#include "SessionResources.h"

/**
 *Provide operator() for all availble commands.
 */
template<class... Ts> struct Overload : Ts... { using Ts::operator()...; };
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;


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
 *Build a command object for the given command ID.
 */
static std::optional<ThreadPool::Commands_t> build_command(const std::string& commandId, const char* line, SessionResources& resources)
{
    if (commandId == "USER")
    {
        return CmdUser(commandId, resources.get_stream(), line, resources.get_user());
    }
    else if (commandId == "WRITE")
    {
        return CmdWrite(commandId, resources.get_stream(), line);
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

        //{
            //open_socket_stream(resources.get_clientSocket(), resources.get_stream());
            //fputs("Hi there!\n", resources.get_stream());
            //print_commands(resources.get_stream());

            //while (fgets(line.data(), 100, resources.get_stream()))
            //{
                //io.str(line.data());
                ////io.str(line);
                //io >> commandId;
                //std::cout << commandId << std::endl;
                //fputs("once more ...\n", resources.get_stream());
            //}
            //fclose(resources.get_stream());
        //}

        try
        {
            open_socket_stream(resources.get_clientSocket(), resources.get_stream());
        }
        catch (const BBServException& error)
        {
            std::cout << "Stop processing client request on socket " << resources.get_clientSocket()
               << ": " << error.what() << std::endl;
            return nullptr;
        }

        print_commands(resources.get_stream());

        while (fgets(line.data(), line.size(), resources.get_stream()))
        {
            debug_print(pool, "Received on ", resources.get_clientSocket(), ": ", line.data());

            sscanf(line.data(), "%s ", commandId.data());

            if (0 == std::strncmp("QUIT", commandId.data(), 4))
            {
                break;
            }

            //auto userCmd { CmdUser(commandId, io, resources.get_user()) };

            //std::vector<ThreadPool::Commands_t> commands =
                //{ userCmd };

            //for (auto& command : commands)
            //{
                //std::visit(Overload {
                        ////[](CmdUser& cmd) { cmd.execute(); }
                        //[](CmdUser&) { std::cout << "It's a USER command" << std::endl;}
                        //}, command);
            //}

            try
            {
                ThreadPool::Commands_t command { build_command(commandId.data(), line.data(), resources).value() };

                std::visit([](auto&& command) { command.execute(); }, command);
            }
            catch (const std::bad_optional_access&)
            {
                std::cout << "ERROR - Failed to build command object from " << line.data() << std::endl;
                break;
            }
            catch (const BBServException& error)
            {
                std::cout << error.what() << std::endl;
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
