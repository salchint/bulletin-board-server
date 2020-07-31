// Filename: ConfigSuite.cpp

#include "Config.h"
#include <fstream>
#include <cstdio>

// Fake implementations

#include "gtest/gtest.h"
//#include "gmock/gmock.h"

#include "Config.h"
#include "Config.cpp"


// Prototyping

// Fixture
class ConfigSuite : public ::testing::Test
{
public:

public:
    ConfigSuite()
    {
    }

public:
    void SetUp() override
    {
        std::ofstream fout ("bbserv.conf");
        fout << "THMAX=10" << std::endl;
        fout << "BBPORT=9100" << std::endl;
        fout << "SYNCPORT=10100" << std::endl;
        fout << "BBFILE=bbfile.db" << std::endl;
        fout << "PEERS=localhost:10200 127.0.0.1:10300" << std::endl;
        fout << "DAEMON=0" << std::endl;
        fout << "DEBUG=true" << std::endl;
    }

    void TearDown() override
    {
        std::remove("bbserv.conf");
    }
};

// Tests
TEST_F(ConfigSuite, testDefaults)
{
    auto& obj = Config::singleton();
    EXPECT_STREQ("", obj.get_bbfile().data());
    EXPECT_EQ(20, obj.get_Tmax());
    EXPECT_EQ(9000, obj.get_bport());
    EXPECT_EQ(10000, obj.get_sport());
    EXPECT_EQ(true, obj.is_daemon());
    EXPECT_EQ(false, obj.is_debug());
}

TEST_F(ConfigSuite, testReadConf)
{
    auto& obj = Config::singleton();
    obj.read_config();
    EXPECT_STREQ("bbfile.db", obj.get_bbfile().data());
    EXPECT_EQ(10, obj.get_Tmax());
    EXPECT_EQ(9100, obj.get_bport());
    EXPECT_EQ(10100, obj.get_sport());
    EXPECT_EQ(false, obj.is_daemon());
    EXPECT_EQ(true, obj.is_debug());
    auto peers = obj.get_peers();
    ASSERT_EQ(2, peers.size());
    EXPECT_EQ("localhost:10200", peers[0].represent());
    EXPECT_EQ("127.0.0.1:10300", peers[1].represent());
}
