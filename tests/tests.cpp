#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <array>


// Fake implementations

#include "gtest/gtest.h"
//#include "gmock/gmock.h"



// Prototyping

// Fixture
class IntegrationSuite : public ::testing::Test
{
public:
    int childId {0};
    int clientSocket {0};
    FILE* stream {nullptr};

public:
    IntegrationSuite()
    {
        constexpr auto exe { "./bbserv"};
        childId = fork();
        if (!childId)
        {
            /**/
            EXPECT_NE(-1, execlp(exe, exe, "-d", nullptr)) << "Failed to launch " << exe;
            /*/
            EXPECT_NE(-1, execlp(exe, exe, nullptr)) << "Failed to launch " << exe;
            /*/
        }
        else
        {
            sleep(1);
        }
    }

    ~IntegrationSuite()
    {
        kill(this->childId, SIGINT);
    }

public:
    void SetUp() override
    {
        struct sockaddr_in clientAddress;
        socklen_t addressSize = sizeof(struct sockaddr_in);
        memset(&clientAddress, 0, addressSize);

        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_LE(0, clientSocket) << "Failed to create socket";

        clientAddress.sin_family = AF_INET;
        clientAddress.sin_addr.s_addr = INADDR_ANY;
        clientAddress.sin_port = htons(9000);
        EXPECT_EQ(0, connect(clientSocket, (struct sockaddr*)&clientAddress, addressSize));

        stream = fdopen(this->clientSocket, "rb+");
        //std::cout << "TEST - Open stream on " << this->clientSocket << std::endl;
        EXPECT_NE(stream, nullptr);
    }

    void TearDown() override
    {
        if (stream)
        {
            fputs("QUIT\n", this->stream);
            fputc('\n', this->stream);
            fclose(this->stream);
            this->stream = nullptr;
            this->clientSocket = 0;
        }
        else if (this->clientSocket)
        {
            close(this->clientSocket);
            this->clientSocket = 0;
        }
    }
};

// Tests
TEST_F(IntegrationSuite, test00)
{
    EXPECT_EQ(1, 1);
}

TEST_F(IntegrationSuite, testGreetings)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    EXPECT_STREQ(buffer.data(), "0.0 bbserv supported commands: [USER <name>|READ <msg-number>|WRITE <msg>|REPLACE <msg-number>/<msg>|QUIT <text>]\n");
}

TEST_F(IntegrationSuite, testUser)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    EXPECT_STREQ(buffer.data(), "0.0 bbserv supported commands: [USER <name>|READ <msg-number>|WRITE <msg>|REPLACE <msg-number>/<msg>|QUIT <text>]\n");
    // Send my name
    fputs("USER Test-bbserv", stream);
    fflush(stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    EXPECT_STREQ(buffer.data(), "1.0 HELLO Test-bbserv I'm ready");
}

