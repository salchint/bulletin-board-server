//Filename:  AcknowledgeQueue.cpp

#include "AcknowledgeQueue.h"
#include "AutoLock.h"


void AcknowledgeQueue::add(bool success) noexcept
{
    AutoLock guard(&queueMutex);

    this->ackQueue.emplace_back(success);

    guard.unlock();
    pthread_cond_signal(&queueCondition);
}

size_t AcknowledgeQueue::check_success(size_t replyCount) noexcept
{
    AutoLock guard(&queueMutex);

    while (this->ackQueue.size() < replyCount)
    {
        pthread_cond_wait(&queueCondition, &queueMutex);
    }

    // Determine if there is at least one negative reply
    for (auto success : this->ackQueue)
    {
        if (!success)
        {
            return false;
        }
    }
    return true;
}
