//Filename:  ThreadPool.h

#pragma once

#include <pthread.h>
#include <vector>
#include <memory>
#include "ConnectionQueue.h"
#include "BBServException.h"

/**
 *A container of agents operating on client requests.
 */
class ThreadPool
{
    protected:
        size_t size {1};
        std::vector<pthread_t> pool;
        std::shared_ptr<ConnectionQueue> connectionQueue;

    public:
        /**
         *Create a new container with a fixed number of agents.
         */
        ThreadPool(size_t size)
            : size(size)
        {
            this->pool.resize(size);
        }

    public:
        /**
         *Start processing all incoming client requests.
         *Upon error this throws BBServException.
         */
        void operate(std::shared_ptr<ConnectionQueue>& qu);

        /**
         *Wait for the next incoming client connection and return the socket.
         */
        int get_connection() noexcept;
};
