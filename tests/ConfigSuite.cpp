// Filename: ConfigSuite.cpp

#include "Config.h"

// Fake implementations

#include "gtest/gtest.h"
//#include "gmock/gmock.h"



// Prototyping

// Fixture
class ConfigSuite : public ::testing::Test {
public:

public:
    ConfigSuite() {
    }

public:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// Tests
TEST_F(ConfigSuite, testConstruction) {
    auto& obj = Config::singleton();
    EXPECT_EQ("", obj.get_bbfile());
    EXPECT_EQ(20, obj.get_Tmax());
    EXPECT_EQ(9000, obj.get_bport());
    EXPECT_EQ(10000, obj.get_sport());
    EXPECT_EQ(true, obj.is_daemon());
    EXPECT_EQ(false, obj.is_debug());
}
