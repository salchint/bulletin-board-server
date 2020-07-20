///Filename:  ThreadPool.cpp

#include "ThreadPool.h"
#include "Config.h"
#include <errno.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <vector>
#include <array>
#include <optional>
#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "SessionResources.h"
#include "CmdBuilder.h"

/**
 *Provide operator() for all availble commands.
 */
template<class... Ts> struct Overload : Ts... { using Ts::operator()...; };
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;


/**
 *Print the supported commands as a welcome message.
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
 * Establish a network connection to the given peer.
 *
 * Returns a socket in non-blocking mode to send the broadcast commands to.
 */
int create_peer_socket(ThreadPool* pool, Peer& peer)
{
    addrinfo hints;
    addrinfo *serverInfo;
    auto peerSocket {-1};

    std::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    auto status { getaddrinfo(peer.host.data(), std::to_string(peer.port).data(),
            &hints, &serverInfo) };

    if (0 != status)
    {
        error_return(pool, "Failed to get the address info of ", peer,
                ": ", gai_strerror(status));
    }

    for (auto info {serverInfo}; info; info = info->ai_next)
    {
        // Check if a socket can be created from the address
        peerSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (-1 == peerSocket)
        {
            // This has not been the right address
            debug_print(pool, "Probing for socket ", peerSocket, " failed");
            peerSocket = -1;
            continue;
        }

        //fcntl(peerSocket, F_SETFL, O_NONBLOCK);

        // In non-blocking mode, first check if the remote host is ready to connect
        pollfd descriptor;
        descriptor.fd = peerSocket;
        descriptor.events = POLLIN;

        auto ready { poll(&descriptor, 1, Config::singleton().get_network_timeout_ms()) };

        if (0 == ready)
        {
            // timeout
            debug_print(pool, "Timeout occurred");
            peerSocket = -1;
            continue;
        }
        else if (-1 == ready)
        {
            // error
            debug_print(pool, "Failed to poll connection on ", peerSocket, "; ",
                    strerror(errno));
            peerSocket = -1;
            continue;
        }

        // Check if the connection can be established finally
        status = connect(peerSocket, info->ai_addr, info->ai_addrlen);
        if (-1 == status)
        {
            // This has not been the right address
            debug_print(pool, "Cannot use socket ", peerSocket, " for connection: ",
                    strerror(errno));
            peerSocket = -1;
            continue;
        }

        // If we come here, we have the right address and the remote host is ready
        debug_print(pool, "Connected socket ", peerSocket, " to peer ", peer);
        break;
    }

    freeaddrinfo(serverInfo);
    debug_print(pool, "Created peer socket ", peerSocket);
    return peerSocket;
}

/**
 *Wait on the given peer socket for some data to arrive.
 */
static void wait_for_data(ThreadPool* pool, int peerSocket, BroadcastCommand& command)
{
    pollfd descriptor;
    descriptor.fd = peerSocket;
    descriptor.events = POLLIN;

    auto ready { poll(&descriptor, 1, Config::singleton().get_network_timeout_ms()) };

    if (0 == ready)
    {
        // timeout
        debug_print(pool, "Timeout occurred");
        timeout_return(pool, "Peer ", command.peer, " did not respond in time");
    }
    else if (-1 == ready)
    {
        // error
        error_return("Failed to poll for incoming data at socket ", peerSocket);
    }
    debug_print(pool, "Data available on socket ", peerSocket);
}

/**
 *Process request to send the given command to the given peer.
 */
static void* process(ThreadPool* pool, SessionResources& resources, BroadcastCommand command)
{
    std::array<char, 1024> line;
    std::array<char, 16> commandId;
    auto read {line.data()};

    debug_print(pool, "Dequeued command to be sent to peer: ", command.command,
            "/", command.peer);

    auto peerSocket { create_peer_socket(pool, command.peer) };

    try
    {
        open_socket_stream(peerSocket, resources.get_stream());

        // In non-blocking mode, first check if there are data to be received.
        debug_print(pool, "Try to read data from ", peerSocket);
        wait_for_data(pool, peerSocket, command);

        read = fgets(line.data(), line.size(), resources.get_stream());
        if (!read)
        {
            error_return(pool, "Failed to read from socket ", peerSocket,
                    strerror(errno)); }
            debug_print(pool, "Received on ", peerSocket, ": ", line.data());

            // Consume and ignore the welcome message
            if (0 == strncmp("0.0", line.data(), 3))
            {
            }

            sscanf(command.command.data(), "%s ", commandId.data());

            auto commandObj { build_command(commandId.data(), command.command.data(),
                    resources, pool->get_connection_queue()).value() };

            std::visit([](auto&& cmd) { cmd.execute(); }, commandObj);

    }
    catch (const std::bad_optional_access&)
    {
        std::cout << "ERROR - Failed to build command object from unknown '"
            << line.data() <<"'" << std::endl;
    }
    catch (const BBServTimeout& timeout)
    {
        std::cout << timeout.what() << std::endl;
        return nullptr;
    }
    catch (const BBServException& error)
    {
        std::cout << error.what() << std::endl;

        return nullptr;
    }

    // Shutdown in a civilized manner
    auto commandQuit { build_command("QUIT", "QUIT", resources, pool->get_connection_queue()).value() };
    std::visit([](auto&& cmd) { cmd.execute(); }, commandQuit);

    debug_print(pool, "Peer connection closed on ", peerSocket);
    return nullptr;
}

/**
 *Process request to handle incoming client connections.
 */
static void* process(ThreadPool* pool, SessionResources& resources, int clientSocket)
{
    std::array<char, 16> commandId;
    std::array<char, 1024> line;

    resources.get_clientSocket() = clientSocket;

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

    while (fgets(line.data(), line.size(), resources.get_stream()))
    {
        debug_print(pool, "Received on ", resources.get_clientSocket(), ": ", line.data());

        sscanf(line.data(), "%s ", commandId.data());

        try
        {
            auto command { build_command(commandId.data(), line.data(), resources, pool->get_connection_queue()).value() };

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

    return nullptr;
}

/**
 *The thread's entry point.
 */
static void* thread_main(void* p)
{
    auto pool { reinterpret_cast<ThreadPool*>(p) };

    for (;;)
    {
        SessionResources resources;
        auto entry { pool->get_entry() };

        try
        {
            std::visit(Overload {
                    [&](int socket) { process(pool, resources, socket); },
                    [&](BroadcastCommand cmd) { process(pool, resources, cmd); },
                    }, entry);
        }
        catch (const BBServException& error)
        {
            std::cout << error.what() << std::endl;
        }
    }
}

/**
 *Create a pool of worker threads.
 */
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

ConnectionQueue::Entry_t ThreadPool::get_entry() noexcept
{
    return this->connectionQueue->get();
}

ConnectionQueue* ThreadPool::get_connection_queue() noexcept
{
    return this->connectionQueue.get();
}
