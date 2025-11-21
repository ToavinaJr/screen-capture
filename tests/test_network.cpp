#include <gtest/gtest.h>
#include "network/TLSConnection.h"
#include "network/StreamServer.h"

class NetworkTest : public ::testing::Test {
protected:
    TLSConnection* tlsConnection;
    StreamServer* streamServer;

    void SetUp() override {
        tlsConnection = new TLSConnection();
        streamServer = new StreamServer();
    }

    void TearDown() override {
        delete tlsConnection;
        delete streamServer;
    }
};

TEST_F(NetworkTest, TestTLSConnectionInitialization) {
    EXPECT_TRUE(tlsConnection->initialize("server.crt", "server.key"));
}

TEST_F(NetworkTest, TestStreamServerStart) {
    EXPECT_TRUE(streamServer->start(8080));
}

TEST_F(NetworkTest, TestStreamServerHandleClient) {
    streamServer->start(8080);
    EXPECT_NO_THROW(streamServer->handleClient());
}