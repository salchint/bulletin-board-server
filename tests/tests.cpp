#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <cstring>
#include <array>
#include <cstdio>
#include <fstream>


// Fake implementations

#include "gtest/gtest.h"
//#include "gmock/gmock.h"



// Prototyping

// Fixture
class IntegrationSuite : public ::testing::Test
{
protected:
    constexpr static const char* const DEFAULT_BBFILE { "bbfile.txt" };
    constexpr static const char* const DEFAULT_NOFILE { "bbfile.txt.no" };

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
            usleep(100000);
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

        // Delete the number file and bbfile
        std::remove(DEFAULT_BBFILE);
        std::remove(DEFAULT_NOFILE);
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
    // Send my name
    fputs("USER Test-bbserv\n", this->stream);
    fflush(this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    EXPECT_STREQ(buffer.data(), "1.0 HELLO Test-bbserv I'm ready\n");
}

TEST_F(IntegrationSuite, testWrite)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    buffer.fill('\0');
    // Send WRITE request
    fputs("WRITE The first line\n", this->stream);
    fflush(this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    EXPECT_STREQ(buffer.data(), "3.0 WROTE 0\n");
}

TEST_F(IntegrationSuite, testWriteFile)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    buffer.fill('\0');
    // Send WRITE request
    fputs("WRITE The first line\n", this->stream);
    fflush(this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    buffer.fill('\0');
    // Verify the file content
    std::ifstream input(DEFAULT_BBFILE);
    EXPECT_TRUE(input.good()) << "Failed to open " << DEFAULT_BBFILE;
    input.get(buffer.data(), buffer.size(), EOF);
    EXPECT_STREQ(buffer.data(), "0/nobody/The first line\n") << DEFAULT_BBFILE << " does not contain the desired line";
}

TEST_F(IntegrationSuite, testWriteFile_multiple)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE A\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE B\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Change user name
    fputs("USER Mike\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE C\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Verify the file content
    std::ifstream input(DEFAULT_BBFILE);
    EXPECT_TRUE(input.good()) << "Failed to open " << DEFAULT_BBFILE;
    input.get(buffer.data(), buffer.size(), EOF);
    EXPECT_STREQ(buffer.data(), "0/nobody/A\n1/nobody/B\n2/Mike/C\n") << DEFAULT_BBFILE << " does not contain the desired lines";
}

TEST_F(IntegrationSuite, testRead)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE The first line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Read the message again
    fputs("READ 0\n", this->stream);
    // Receive message text
    fgets(buffer.data(), buffer.size(), this->stream);
    // Verify the read message
    EXPECT_STREQ(buffer.data(), "2.0 MESSAGE 0 nobody/The first line\n") << "Failed to read the just written message";
}

TEST_F(IntegrationSuite, testRead_wrongId)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE The first line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Read the message again
    fputs("READ 1\n", this->stream);
    // Receive message text
    fgets(buffer.data(), buffer.size(), this->stream);
    // Verify the read message
    EXPECT_STREQ(buffer.data(), "2.1 UNKNOWN 1 Record not found\n") << "The message ID 1 should be unknown";
}

TEST_F(IntegrationSuite, testRead_noBBFile)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    // Read the message again
    fputs("READ 0\n", this->stream);
    // Receive message text
    fgets(buffer.data(), buffer.size(), this->stream);
    // Verify the read message
    EXPECT_NE(nullptr, std::strstr(buffer.data(), "2.2 ERROR READ")) << "The bbfile should be not written yet";
    EXPECT_NE(nullptr, std::strstr(buffer.data(), "bbfile is not available")) << "The bbfile should be not written yet";
}

TEST_F(IntegrationSuite, testRead_malformed)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE The first line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Issue a malformed READ request
    fputs("READ a\n", this->stream);
    // Receive an error
    fgets(buffer.data(), buffer.size(), this->stream);
    // Verify the read message
    EXPECT_NE(nullptr, std::strstr(buffer.data(), "2.2 ERROR READ"))
        << "The READ request shall contain a message ID: " << buffer.data();
    EXPECT_NE(nullptr, std::strstr(buffer.data(), "Request malformed"))
        << "The READ request shall contain a message ID: " << buffer.data();
}

TEST_F(IntegrationSuite, testReplace)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE Headline\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE The first line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Write another record
    fputs("WRITE The second line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Write another record
    fputs("REPLACE 1/The initial line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Verify the replace message
    EXPECT_STREQ(buffer.data(), "3.0 WROTE 1\n");
    // Verify the file content
    std::ifstream input(DEFAULT_BBFILE);
    EXPECT_TRUE(input.good()) << "Failed to open " << DEFAULT_BBFILE;
    input.get(buffer.data(), buffer.size(), EOF);
    EXPECT_STREQ(buffer.data(), "0/nobody/Headline\n1/nobody/The initial line\n2/nobody/The second line\n")
        << DEFAULT_BBFILE << " does not contain the desired line";
}

TEST_F(IntegrationSuite, testReplace_changeUser)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE Headline\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE The first line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Write another record
    fputs("WRITE The second line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Change user
    fputs("USER Mike\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Replace the middle record
    fputs("REPLACE 1/The initial line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Verify the replace message
    EXPECT_STREQ(buffer.data(), "3.0 WROTE 1\n");
    // Verify the file content
    std::ifstream input(DEFAULT_BBFILE);
    EXPECT_TRUE(input.good()) << "Failed to open " << DEFAULT_BBFILE;
    input.get(buffer.data(), buffer.size(), EOF);
    EXPECT_STREQ(buffer.data(), "0/nobody/Headline\n1/Mike/The initial line\n2/nobody/The second line\n")
        << DEFAULT_BBFILE << " does not contain the desired line";
}

TEST_F(IntegrationSuite, testReplace_unknownId)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE Headline\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE The first line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Write another record
    fputs("WRITE The second line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Write another record
    fputs("REPLACE 3/The initial line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    // Verify the replace message
    EXPECT_STREQ(buffer.data(), "3.1 UNKNOWN 3\n");
    // Verify the unchanged file content
    std::ifstream input(DEFAULT_BBFILE);
    EXPECT_TRUE(input.good()) << "Failed to open " << DEFAULT_BBFILE;
    input.get(buffer.data(), buffer.size(), EOF);
    EXPECT_STREQ(buffer.data(), "0/nobody/Headline\n1/nobody/The first line\n2/nobody/The second line\n")
        << DEFAULT_BBFILE << " does not contain the desired line";
}

