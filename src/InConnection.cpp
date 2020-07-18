#include "InConnection.h"
#include <cstring>
#include <memory>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include "Config.h"

/**
 *Data passed on to the thread/context.
 */
struct ContextData
{
    ContextData(InConnection* self, in_port_t port)
        : self(self)
        , port(port)
    {};

    InConnection* self {nullptr};
    in_port_t port {0};
};

/**
 *The thread's main entry point.
 */
static void* thread_main(void* p)
{
    auto contextData { reinterpret_cast<ContextData*>(p) };
    auto port { contextData->port };
    auto self { contextData->self };
    delete contextData;

    self->listen_on(port);

    return nullptr;
}

InConnection::InConnection(std::shared_ptr<ConnectionQueue>& qu)
{
    this->connectionQueue = qu;
}

InConnection::~InConnection()
{
    if (this->acceptSocket) {
       close(this->acceptSocket);
       this->acceptSocket = 0;
    }
}

void InConnection::listen_on(/*const std::string_view& ipaddress,*/ in_port_t port)
{
    open_incoming_conn(/*ipaddress,*/ port);
    listen_for_clients();
}

void InConnection::operate(/*const std::string_view& ipaddress,*/ in_port_t port)
{
    pthread_t context;
    pthread_attr_t contextOptions;
    pthread_attr_init(&contextOptions);
    pthread_attr_setdetachstate(&contextOptions, PTHREAD_CREATE_DETACHED);

    auto contextData = new ContextData(this, port);

    bool success { 0 == pthread_create(&context, &contextOptions,
            thread_main, reinterpret_cast<void*>(contextData))};

    if (!success)
    {
        error_return(this, "Failed to create context: ", strerror(errno));
    }

    pthread_attr_destroy(&contextOptions);
    debug_print(this, "Context created and ready");
}

/**
 *Create and bind a socket to any of this host's network interfaces.
 */
void InConnection::open_incoming_conn(/*const std::string_view& ipaddress,*/ in_port_t port)
{
    using std::string_view;

    int enable = 1;
    struct sockaddr_in acceptAddress;
    socklen_t addressSize = sizeof(struct sockaddr_in);

    memset(&acceptAddress, 0, addressSize);

    this->acceptSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > this->acceptSocket)
    {
        error_return(this, "Failed to connect to socket for incoming connections.");
    }
    setsockopt(this->acceptSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&enable,
            sizeof(enable));

    if (this->connectionQueue->is_nonblocking())
    {
        fcntl(this->acceptSocket, F_SETFL, O_NONBLOCK);
    }

    //if (!inet_aton(ipaddress.data(), &acceptAddress.sin_addr))
    //{
        //error_return(this, "Invalid IP-address");
    //}

    acceptAddress.sin_port = htons(port);
    acceptAddress.sin_family = AF_INET;
    acceptAddress.sin_addr.s_addr = INADDR_ANY;
    if (0 != bind(this->acceptSocket, (struct sockaddr*)(&acceptAddress),
            addressSize)) {
        //error_return(this, "Failed to bind socket to ", ipaddress);
        error_return(this, "Failed to bind socket to 0.0.0.0:", port);
    }

    debug_print(this, "Created socket and bound it to ", inet_ntoa(acceptAddress.sin_addr), ":", port);
}

/**
 * Listen at the socket, accept incoming connections and delegate them to a worker thread.
 *
 * In case this instance is operating on a socket in non-blocking mode, it
 * might throw BBServTimeout.
 */
void InConnection::listen_for_clients()
{
    auto clientSocket {0};
    auto timeoutms { this->connectionQueue->get_timeout_ms() };

    listen(this->acceptSocket, Config::singleton().get_Tmax());
    debug_print(this, "Listening for incoming messages");

    while (1) {
        // Wait a limited ammount of time for incoming connections if the
        // socket is in non-blocking mode.
        if (timeoutms)
        {
            pollfd descriptor;
            descriptor.fd = this->acceptSocket;
            descriptor.events = POLLIN;

            auto ready { poll(&descriptor, 1, *timeoutms) };

            if (0 == ready)
            {
                // timeout
                timeout_return(this, "Timeout at waiting for incoming network connection");
            }
            else if (-1 == ready)
            {
                // error
                error_return(this, "Failed to poll socket ", this->acceptSocket);
            }
        }

        clientSocket = accept(this->acceptSocket, NULL, NULL);
        if (0 > clientSocket) {
            continue;
        }

        debug_print(this, "Accepted client connection on ", clientSocket);
        this->connectionQueue->add(clientSocket);

        // Don't keep on waiting for other connections in non-blocking mode.
        if (timeoutms)
        {
            break;
        }
    }

    close(acceptSocket);
}

