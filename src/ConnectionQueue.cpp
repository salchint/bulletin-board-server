//Filename:  ConnectionQueue.cpp

#include "ConnectionQueue.h"
#include "AutoLock.h"


void ConnectionQueue::add(int clientSocket)
{
    AutoLock guard(&queueMutex);

    this->connectionQueue.emplace(clientSocket);

    guard.unlock();
    pthread_cond_signal(&queueCondition);
}

int ConnectionQueue::get()
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
