#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <cstring>
#include <array>
#include <vector>
#include <cstdio>
#include <fstream>


// Fake implementations

#include "gtest/gtest.h"
//#include "gmock/gmock.h"



// Prototyping

// Fixture
class ReplicationSuite : public ::testing::Test
{
public:
    std::vector<int> childIds;
    int clientSocket {0};
    FILE* stream {nullptr};

public:
    ReplicationSuite()
    {
        constexpr auto exe { "./bbserv"};
        childIds.push_back(fork());
        if (!childIds[childIds.size()-1])
        {
            EXPECT_NE(-1, execlp(exe, exe, "-d", "-bdb.10100.txt", "-p9100", "-s10100", "127.0.0.1:10200", nullptr)) << "Failed to launch " << exe;
        }
        else
        {
            childIds.push_back(fork());
            if (!childIds[childIds.size()-1])
            {
                EXPECT_NE(-1, execlp(exe, exe, "-d", "-bdb.10200.txt", "-p9200", "-s10200", "127.0.0.1:10100", nullptr)) << "Failed to launch " << exe;
            }
            else
            {
                // TODO
                usleep(2000000);
            }
        }
    }

    ~ReplicationSuite()
    {
        for (auto childId : this->childIds)
        {
            kill(childId, SIGINT);
        }
        usleep(500000);
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
        clientAddress.sin_port = htons(9100);
        EXPECT_EQ(0, connect(clientSocket, (struct sockaddr*)&clientAddress, addressSize))
            << strerror(errno);

        stream = fdopen(this->clientSocket, "rb+");
        EXPECT_NE(stream, nullptr);

        // Delete the number file and bbfile
        std::remove("db.10100.txt");
        std::remove("db.10200.txt");
        std::remove("db.10100.txt.no");
        std::remove("db.10200.txt.no");
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
TEST_F(ReplicationSuite, test00)
{
    EXPECT_EQ(1, 1);
}

TEST_F(ReplicationSuite, testWriteFile)
{
    std::array<char, 1024> buffer;
    // Receive greetings
    fgets(buffer.data(), buffer.size(), this->stream);
    // Send WRITE request
    fputs("WRITE The first line\n", this->stream);
    // Receive acknowledge
    fgets(buffer.data(), buffer.size(), this->stream);
    EXPECT_STREQ(buffer.data(), "3.0 WROTE 0\n");
    // TODO
    usleep(2000000);
    // Verify the file content of the sibling
    std::ifstream input("db.10200.txt");
    EXPECT_TRUE(input.good()) << "Failed to open " << "db.10200.txt";
    memset(buffer.data(), 0, buffer.size());
    input.get(buffer.data(), buffer.size(), EOF);
    EXPECT_STREQ(buffer.data(), "0/nobody/The first line\n") << "db.10200.txt" << " does not contain the desired line";
}

TEST_F(ReplicationSuite, testWriteFile_multipleUser)
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
    std::ifstream input("db.10200.txt");
    EXPECT_TRUE(input.good()) << "Failed to open " << "db.10200.txt";
    input.get(buffer.data(), buffer.size(), EOF);
    EXPECT_STREQ(buffer.data(), "0/nobody/A\n1/nobody/B\n2/Mike/C\n") << "db.10200.txt" << " does not contain the desired lines";
}

//TEST_F(ReplicationSuite, testRead)
//{
    //std::array<char, 1024> buffer;
    //// Receive greetings
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Send WRITE request
    //fputs("WRITE The first line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Read the message again
    //fputs("READ 0\n", this->stream);
    //// Receive message text
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Verify the read message
    //EXPECT_STREQ(buffer.data(), "2.0 MESSAGE 0 nobody/The first line\n") << "Failed to read the just written message";
//}

//TEST_F(ReplicationSuite, testRead_wrongId)
//{
    //std::array<char, 1024> buffer;
    //// Receive greetings
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Send WRITE request
    //fputs("WRITE The first line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Read the message again
    //fputs("READ 1\n", this->stream);
    //// Receive message text
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Verify the read message
    //EXPECT_STREQ(buffer.data(), "2.1 UNKNOWN 1 Record not found\n") << "The message ID 1 should be unknown";
//}

//TEST_F(ReplicationSuite, testRead_noBBFile)
//{
    //std::array<char, 1024> buffer;
    //// Receive greetings
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Read the message again
    //fputs("READ 0\n", this->stream);
    //// Receive message text
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Verify the read message
    //EXPECT_NE(nullptr, std::strstr(buffer.data(), "2.2 ERROR READ")) << "The bbfile should be not written yet";
    //EXPECT_NE(nullptr, std::strstr(buffer.data(), "bbfile is not available")) << "The bbfile should be not written yet";
//}

//TEST_F(ReplicationSuite, testRead_malformed)
//{
    //std::array<char, 1024> buffer;
    //// Receive greetings
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Send WRITE request
    //fputs("WRITE The first line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Issue a malformed READ request
    //fputs("READ a\n", this->stream);
    //// Receive an error
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Verify the read message
    //EXPECT_NE(nullptr, std::strstr(buffer.data(), "2.2 ERROR READ"))
        //<< "The READ request shall contain a message ID: " << buffer.data();
    //EXPECT_NE(nullptr, std::strstr(buffer.data(), "Request malformed"))
        //<< "The READ request shall contain a message ID: " << buffer.data();
//}

//TEST_F(ReplicationSuite, testReplace)
//{
    //std::array<char, 1024> buffer;
    //// Receive greetings
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Send WRITE request
    //fputs("WRITE Headline\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Send WRITE request
    //fputs("WRITE The first line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Write another record
    //fputs("WRITE The second line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Write another record
    //fputs("REPLACE 1/The initial line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Verify the replace message
    //EXPECT_STREQ(buffer.data(), "3.0 WROTE 1\n");
    //// Verify the file content
    //std::ifstream input(DEFAULT_BBFILE);
    //EXPECT_TRUE(input.good()) << "Failed to open " << DEFAULT_BBFILE;
    //input.get(buffer.data(), buffer.size(), EOF);
    //EXPECT_STREQ(buffer.data(), "0/nobody/Headline\n1/nobody/The initial line\n2/nobody/The second line\n")
        //<< DEFAULT_BBFILE << " does not contain the desired line";
//}

//TEST_F(ReplicationSuite, testReplace_changeUser)
//{
    //std::array<char, 1024> buffer;
    //// Receive greetings
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Send WRITE request
    //fputs("WRITE Headline\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Send WRITE request
    //fputs("WRITE The first line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Write another record
    //fputs("WRITE The second line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Change user
    //fputs("USER Mike\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Replace the middle record
    //fputs("REPLACE 1/The initial line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Verify the replace message
    //EXPECT_STREQ(buffer.data(), "3.0 WROTE 1\n");
    //// Verify the file content
    //std::ifstream input(DEFAULT_BBFILE);
    //EXPECT_TRUE(input.good()) << "Failed to open " << DEFAULT_BBFILE;
    //input.get(buffer.data(), buffer.size(), EOF);
    //EXPECT_STREQ(buffer.data(), "0/nobody/Headline\n1/Mike/The initial line\n2/nobody/The second line\n")
        //<< DEFAULT_BBFILE << " does not contain the desired line";
//}

//TEST_F(ReplicationSuite, testReplace_unknownId)
//{
    //std::array<char, 1024> buffer;
    //// Receive greetings
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Send WRITE request
    //fputs("WRITE Headline\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Send WRITE request
    //fputs("WRITE The first line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Write another record
    //fputs("WRITE The second line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Write another record
    //fputs("REPLACE 3/The initial line\n", this->stream);
    //// Receive acknowledge
    //fgets(buffer.data(), buffer.size(), this->stream);
    //// Verify the replace message
    //EXPECT_STREQ(buffer.data(), "3.1 UNKNOWN 3\n");
    //// Verify the unchanged file content
    //std::ifstream input(DEFAULT_BBFILE);
    //EXPECT_TRUE(input.good()) << "Failed to open " << DEFAULT_BBFILE;
    //input.get(buffer.data(), buffer.size(), EOF);
    //EXPECT_STREQ(buffer.data(), "0/nobody/Headline\n1/nobody/The first line\n2/nobody/The second line\n")
        //<< DEFAULT_BBFILE << " does not contain the desired line";
//}

