// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string.h>

#include <string>

#include "xpbase/base/top_log.h"

#include <gtest/gtest.h>

#define private public
#include "xtransport/udp_transport/udp_transport.h"
#include "xtransport/message_manager/multi_message_handler.h"

namespace top {
namespace kadmlia {
namespace test {

class TestUdpServer : public testing::Test {
public:
	static void SetUpTestCase() {
	}

	static void TearDownTestCase() {
	}

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}
};

TEST_F(TestUdpServer, Handshake) {
    top::transport::UdpTransportPtr udp_transport;
    udp_transport.reset(new top::transport::UdpTransport());
    auto thread_message_handler = std::make_shared<transport::MultiThreadHandler>();
    thread_message_handler->Init();
    ASSERT_TRUE(udp_transport->Start(
            "127.0.0.1",
            0,
            thread_message_handler.get()) == top::transport::kTransportSuccess);
    auto local_port = udp_transport->local_port();
    transport::protobuf::RoutingMessage message;
    base::xpacket_t packet;
    std::string msg;
    if (!message.SerializeToString(&msg)) {
        TOP_INFO("RoutingMessage SerializeToString failed!");
        return;
    }
    xbyte_buffer_t xdata{msg.begin(), msg.end()};
    udp_transport->SendData(xdata, "127.0.0.1", local_port);
    udp_transport->SendData(packet);
    udp_transport->Stop();
}

}  // namespace test
}  // namespace kadmlia
}  // namespace top
