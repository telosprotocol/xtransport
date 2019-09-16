// routing table demo

#include <signal.h>
#include <iostream>
#include <thread>  // NOLINT

#include "xpbase/base/top_config.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#include "xpbase/base/kad_key/get_kadmlia_key.h"
#include "xtransport/transport.h"
#include "xtransport/udp_transport/udp_transport.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xtransport/transport_message_register.h"
#include "xtransport/message_manager/message_manager_intf.h"

namespace top {

std::string global_node_id = RandomString(256);
std::string global_node_id_hash("");
std::shared_ptr<base::KadmliaKey> global_xid;
uint32_t gloabl_platform_type = kPlatform;
std::shared_ptr<top::transport::MultiThreadHandler> multi_thread_message_handler = nullptr;
static const uint32_t kTestTransportMessage = 10;
uint64_t global_b_time = 0;
std::mutex test_mutex;
static const uint32_t kTestNum = 1000000u;
std::atomic<uint32_t> receive_count(0);

int32_t recv(
        uint64_t from_xip_addr_low,
        uint64_t from_xip_addr_high,
        uint64_t to_xip_addr_low,
        uint64_t to_xip_addr_high,
        base::xpacket_t& packet,
        int32_t cur_thread_id,
        uint64_t timenow_ms,
        base::xendpoint_t* from_parent_end) {
    multi_thread_message_handler->HandleMessage(packet);
    return 0;
}

bool CreateTransport(
        const top::base::Config& config,
        transport::UdpTransportPtr& udp_transport) {
    udp_transport.reset(new top::transport::UdpTransport());
    std::string local_ip;
    if (!config.Get("node", "local_ip", local_ip)) {
        TOP_ERROR("get node local_ip from config failed!");
        return false;
    }

    uint16_t local_port = 0;
    config.Get("node", "local_port", local_port);
    multi_thread_message_handler = std::make_shared<top::transport::MultiThreadHandler>();
    multi_thread_message_handler->Init();
    if (udp_transport->Start(
            local_ip,
            local_port,
            multi_thread_message_handler.get()) != transport::kTransportSuccess) {
        TOP_ERROR("start local udp transport failed!");
        return false;
    }
//    udp_transport->register_on_receive_callback(std::bind(
//            recv,
//            std::placeholders::_1,
//            std::placeholders::_2,
//            std::placeholders::_3,
//            std::placeholders::_4,
//            std::placeholders::_5,
//            std::placeholders::_6,
//            std::placeholders::_7,
//            std::placeholders::_8));
    return true;
}

void SignalCatch(int sig_no) {
    if (SIGTERM == sig_no || SIGINT == sig_no) {
        _Exit(0);
    }
}

void TestTransportMessage(
        transport::UdpTransportPtr& transport,
        const std::string& ip,
        uint16_t port) {
    auto b_time = GetCurrentTimeMsec();
    for (uint32_t i = 0; i < kTestNum; ++i) {
        transport::protobuf::RoutingMessage pbft_message;
        pbft_message.set_des_node_id("");
        pbft_message.set_type(kTestTransportMessage);
        pbft_message.set_id(10);
        pbft_message.set_broadcast(true);
        std::string data;
        pbft_message.SerializeToString(&data);
        transport->SendData({ data.begin(), data.end() }, ip, port);
    }
    auto use_time_ms = double(GetCurrentTimeMsec() - b_time) / 1000.0;
    std::cout << "send " << kTestNum<< " use time: " << use_time_ms
        << " sec. QPS: " << (uint32_t)((double)kTestNum / use_time_ms) << std::endl;
}

void TestTransportMessageHandler(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    if (global_b_time == 0) {
        std::unique_lock<std::mutex> lock(test_mutex);
        if (global_b_time == 0) {
            global_b_time = GetCurrentTimeMsec();
        }
    }
    ++receive_count;
    if (receive_count % 10000 == 0) {
        auto use_time_ms = double(GetCurrentTimeMsec() - global_b_time) / 1000.0;
        std::cout << "receive " << receive_count << " use time: " << use_time_ms
            << " sec. QPS: " << (uint32_t)((double)receive_count / use_time_ms) << std::endl;
    }
}

}  // namespace top

int main(int argc, char ** argv)
{
    if (signal(SIGTERM, top::SignalCatch) == SIG_ERR ||
            signal(SIGINT, top::SignalCatch) == SIG_ERR) {
        return 1;
    }

    const pid_t current_sys_process_id = getpid();
    std::string init_log_file("/tmp/transferdemo");
    init_log_file += top::base::xstring_utl::tostring((int32_t)current_sys_process_id);
    init_log_file += ".log";
    
    xinit_log(init_log_file.c_str(), true, true);
    xset_log_level(enum_xlog_level_info);
    top::base::Config config;
    if(argc > 1)
    {
        const std::string config_file = std::string(argv[1]);
        if (!config.Init(config_file.c_str())) {
            std::cout << "init config file failed: " << config_file << std::endl;
            return 1;
        }
    }
    else
    {
        if (!config.Init("./conf/transferdemo.conf")) {
            std::cout << "init config file failed: ./transferdemo.conf" << std::endl;
            return 1;
        }
    }
 
    top::global_xid = top::base::GetKadmliaKey();
    top::transport::UdpTransportPtr udp_transport;
    if (!top::CreateTransport(config, udp_transport)) {
        assert(0);
    }

    // register gossip message handler
    top::transport::MessageManagerIntf::Instance()->RegisterMessageProcessor(
            top::kTestTransportMessage,
            top::TestTransportMessageHandler);

    bool first_node = false;
    if (!config.Get("node", "first_node", first_node) || !first_node) {
        std::string peer_ip;
        uint16_t peer_port;
        if (config.Get("node", "peer_ip", peer_ip) && config.Get("node", "peer_port", peer_port)) {
            top::TestTransportMessage(udp_transport, peer_ip, peer_port);
        }
    }
    std::cout << "transport server start ok." << std::endl;
    while (true) {
        std::cout << "receive_count: " << top::receive_count << std::endl;
        sleep(10);
    }
}
