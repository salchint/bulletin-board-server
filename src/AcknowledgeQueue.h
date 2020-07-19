//Filename:  AcknowledgeQueue.h

#pragma once

#include <pthread.h>
#include <vector>
#include "Config.h"

/**
 *This queue serves reply messages from broadcasts sent out to peers.
 */
class AcknowledgeQueue
{
    protected:
        std::vector<bool> ackQueue;

    protected:
        pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t queueCondition = PTHREAD_COND_INITIALIZER;

    public:
        /**
         * Add the reply of a peer to this queue.
         *
         * This function is thread-safe and may be used from all the agents.
         */
        void add(bool success) noexcept;

        /**
         * Check if all the replies are positive.
         *
         * This function is thread-safe and may be used from all the agents.
         */
        size_t check_success(size_t replyCount) noexcept;

};
