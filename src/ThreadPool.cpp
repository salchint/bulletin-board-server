//Filename:  ThreadPool.cpp

#include "ThreadPool.h"
#include "Config.h"
#include <errno.h>
#include <string>
#include <cstring>
#include <cstdio>

static void print_commands(FILE* stream)
{
    fputs("0.0 bbserv supported commands: [USER <name>|READ <msg-number>|WRITE <msg>|REPLACE <msg-number>/<msg>|QUIT <text>]\n", stream);
    fflush(stream);
}

static void open_socket_stream(int socketNumber, FILE** stream)
{
    *stream = fdopen(socketNumber, "rb+");

    if (!*stream) {
        error_return(stream, "Failed to open stream for socket ", socketNumber);
    }
}

static void* thread_main(void* p)
{
    auto pool { reinterpret_cast<ThreadPool*>(p) };
    auto clientSocket { 0 };
    FILE* stream { nullptr };
    std::string buffer;
    buffer.resize(1024);

    for (;;)
    {
        clientSocket = pool->get_connection();

        try
        {
            open_socket_stream(clientSocket, &stream);
        }
        catch (const BBServException& error)
        {
            std::cout << "Stop processing client request on socket " << clientSocket
               << ": " << error.what() << std::endl;
            return nullptr;
        }

        print_commands(stream);
        while (fgets(buffer.data(), buffer.size(), stream))
        {
            debug_print(pool, "Received on ", clientSocket, ": ", buffer);
        }
        debug_print(pool, "Client connection closed on ", clientSocket);

        //if (0 == strncmp("log", planesLog[loggedPlanes], 3)) {
            //control_sort_plane_log(planesLog, loggedPlanes);

            //for (i = 0; i < loggedPlanes; i++) {
                //fprintf(streamToPlane, "%s", planesLog[i]);
            //}
            //fputs(".\n", streamToPlane);
        //} else {
            //loggedPlanes += 1;
            //fprintf(streamToPlane, "%s\n", info);
        //}
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
