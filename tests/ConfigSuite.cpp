#include "config.h"

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
    EXPECT_EQ(4, obj.get_Tmax());
    EXPECT_EQ(0, obj.get_bport());
    EXPECT_EQ(0, obj.get_sport());
    EXPECT_EQ(true, obj.is_daemon());
    EXPECT_EQ(false, obj.is_debug());
}