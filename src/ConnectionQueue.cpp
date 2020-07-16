//Filename:  ConnectionQueue.cpp

#include "ConnectionQueue.h"
#include "AutoLock.h"


void ConnectionQueue::add(int clientSocket) noexcept
{
    AutoLock guard(&queueMutex);

    this->connectionQueue.emplace(clientSocket);

    guard.unlock();
    pthread_cond_signal(&queueCondition);
}

int ConnectionQueue::get() noexcept
{
    AutoLock guard(&queueMutex);

    while (this->connectionQueue.empty())
    {
        pthread_cond_wait(&queueCondition, &queueMutex);
    }

    int clientSocket { this->connectionQueue.front() };
    this->connectionQueue.pop();
    return clientSocket;
}

bool ConnectionQueue::is_nonblocking() const noexcept
{
    return this->isNonblocking;
}

std::optional<int> ConnectionQueue::get_timeout_ms() const noexcept
{
    if (is_nonblocking())
    {
        return 5000;  // ms
    }
    return {};
}
