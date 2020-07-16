//Filename:  ConnectionQueue.h

#pragma once

#include <pthread.h>
#include <queue>
#include <optional>

/**
 *This queue serves incoming client connections to the preallocated threads.
 */
class ConnectionQueue
{
    protected:
        bool isNonblocking {false};
        std::queue<int> connectionQueue;

    protected:
        pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t queueCondition = PTHREAD_COND_INITIALIZER;

    public:
        ConnectionQueue(bool isNonblocking)
            : isNonblocking(isNonblocking)
        {}

    public:
        /**
         * Add the socket of an incoming connection to this queue.
         *
         * This funciton is thread-safe and may be used from all the agnets.
         */
        void add(int clientSocket) noexcept;

        /**
         * Get the socket of an incoming connection in a blocking manner.
         *
         * This funciton is thread-safe and may be used from all the agnets.
         */
        int get() noexcept;

        /**
         * Determines if the network connection, which feeds this queue is run in non-blocking mode.
         *
         * This funciton is thread-safe and may be used from all the agnets.
         */
        bool is_nonblocking() const noexcept;

        /**
         * Get the standard timeout for socket operations if non-blocking is selected.
         *
         * This funciton is thread-safe and may be used from all the agnets.
         */
        std::optional<int> get_timeout_ms() const noexcept;
};
