//Filename:  RWLock.cpp

#include "RWLock.h"
#include "AutoLock.h"

void RWLock::aquire_read()
{
    AutoLock guard (&this->conditionLock);

    // Stop here if there is a write operation in progress or pending
    if (0 < this->writerCount || 0 < this->wWaitCount)
    {
        this->rWaitCount++;
        pthread_cond_wait(&this->readFlag, &this->conditionLock);
        this->rWaitCount--;
    }

    // Read access granted
    this->readerCount++;
    pthread_cond_broadcast(&this->readFlag);
}

void RWLock::release_read()
{
    AutoLock guard (&this->conditionLock);

    // Signal a possibly pending writer
    if (0 == this->readerCount--)
    {
        pthread_cond_signal(&this->writeFlag);
    }
}

void RWLock::aquire_write()
{
    AutoLock guard (&this->conditionLock);

    // Stop here if there is a write or read operation in progress
    if (0 < this->writerCount || 0 < this->readerCount)
    {
        this->wWaitCount++;
        pthread_cond_wait(&this->writeFlag, &this->conditionLock);
        this->wWaitCount--;
    }

    // Write access granted
    this->writerCount++;
    pthread_cond_broadcast(&this->readFlag);
}

void RWLock::release_write()
{
    AutoLock guard (&this->conditionLock);
    this->writerCount--;

    // Signal a possibly pending writer
    if (0 < this->rWaitCount)
    {
        pthread_cond_signal(&this->readFlag);
    }
    else
    {
        pthread_cond_signal(&this->writeFlag);
    }
}

