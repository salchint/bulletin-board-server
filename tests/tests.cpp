#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


// Fake implementations

#include "gtest/gtest.h"
//#include "gmock/gmock.h"



// Prototyping

// Fixture
class A4Suite : public ::testing::Test {
public:

public:
    A4Suite() {
    }

public:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// Tests
TEST_F(A4Suite, test00) {
    EXPECT_EQ(1, 1);
}
