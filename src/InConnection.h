// InConnection.h
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <memory>
#include <utility>
#include <string_view>
#include "ConnectionQueue.h"

#pragma once

/**
 *RAII wrapper for the socket.
 */
class ConnectionResources
{
    int acceptSocket {0};

    public:
    ConnectionResources()
        : acceptSocket(0)
    { }

    ~ConnectionResources()
    {
        if (0 < this->acceptSocket)
        {
            debug_print(this, "Close connection at socket ",
                    this->acceptSocket);
            close(this->acceptSocket);
            this->acceptSocket = 0;
        }
    }

    ConnectionResources(const ConnectionResources& other)
        : acceptSocket(other.acceptSocket)
    { }

    ConnectionResources(ConnectionResources&& other) noexcept
        : acceptSocket(std::exchange(other.acceptSocket, 0))
    { }

    ConnectionResources& operator=(const ConnectionResources& other)
    {
        return *this = ConnectionResources(other);
    }

    ConnectionResources& operator=(ConnectionResources&& other) noexcept
    {
        std::swap(this->acceptSocket, other.acceptSocket);
        return *this;
    }

    public:
    void set_accept_socket(int acceptSocket)
    {
        debug_print(this, "!!! Set socket ", acceptSocket);
        this->acceptSocket = acceptSocket;
    }

    int get_accept_socket()
    {
        return this->acceptSocket;
    }

};

/**
 *Network connection listener.
 */
class InConnection
{
    protected:
        ConnectionResources resources;
        std::shared_ptr<ConnectionQueue> connectionQueue;
        bool isNonblocking {false};

    public:
        InConnection(std::shared_ptr<ConnectionQueue>& qu, bool isNonblocking = false);

    public:
        /**
         * Creates and listens on a socket bound to the given port.
         *
         * Upon error might throw BBServException.
         */
        void listen_on(/*const std::string_view& ipaddress,*/ in_port_t port);

        /**
         *Start working in your own thread context.
         *
         * Upon error might throw BBServException.
         */
        void operate(/*const std::string_view& ipaddress,*/ in_port_t port);

    protected:
        void open_incoming_conn(/*const std::string_view& ipaddress,*/ in_port_t port);
        void listen_for_clients();
        bool is_nonblocking();
};
